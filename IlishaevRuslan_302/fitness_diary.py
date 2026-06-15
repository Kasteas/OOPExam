from __future__ import annotations

from abc import ABC, abstractmethod
from enum import Enum
import json
from pathlib import Path
from typing import Any

import flet as ft


class WorkoutStatus(Enum):
    PLANNED = "запланировано"
    DONE = "выполнено"
    SKIPPED = "пропущено"


class FitnessError(Exception):
    pass


class FitnessValidationError(FitnessError):
    pass


class WorkoutNotFoundError(FitnessError):
    pass


class Workout(ABC):
    workout_type = "base"

    def __init__(self, workout_id: int, title: str, minutes: int, status: WorkoutStatus = WorkoutStatus.PLANNED):
        self.id = workout_id
        self._title = ""
        self.title = title
        self.minutes = int(minutes)
        self.status = status

        if self.minutes <= 0:
            raise FitnessValidationError("Минуты должны быть больше нуля")

    @property
    def title(self) -> str:
        return self._title

    @title.setter
    def title(self, value: str) -> None:
        if not value or not value.strip():
            raise FitnessValidationError("Название тренировки пустое")
        self._title = value.strip()

    def mark_done(self) -> None:
        self.status = WorkoutStatus.DONE

    def mark_skipped(self) -> None:
        self.status = WorkoutStatus.SKIPPED

    @abstractmethod
    def calories(self) -> int:
        pass

    @abstractmethod
    def short_info(self) -> str:
        pass

    def to_dict(self) -> dict[str, Any]:
        return {
            "type": self.workout_type,
            "id": self.id,
            "title": self.title,
            "minutes": self.minutes,
            "status": self.status.name,
        }


class CardioWorkout(Workout):
    workout_type = "cardio"

    def __init__(self, workout_id: int, title: str, minutes: int, intensity: int, status: WorkoutStatus = WorkoutStatus.PLANNED):
        super().__init__(workout_id, title, minutes, status)
        self.intensity = int(intensity)

    def calories(self) -> int:
        return self.minutes * self.intensity * 4

    def short_info(self) -> str:
        return f"Кардио: {self.title}, {self.minutes} мин, интенсивность {self.intensity} | {self.calories()} ккал | {self.status.value}"

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["intensity"] = self.intensity
        return data


class StrengthWorkout(Workout):
    workout_type = "strength"

    def __init__(self, workout_id: int, title: str, minutes: int, muscle_group: str, status: WorkoutStatus = WorkoutStatus.PLANNED):
        super().__init__(workout_id, title, minutes, status)
        self.muscle_group = muscle_group.strip() or "общая"

    def calories(self) -> int:
        return self.minutes * 6

    def short_info(self) -> str:
        return f"Силовая: {self.title}, группа: {self.muscle_group}, {self.minutes} мин | {self.calories()} ккал | {self.status.value}"

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["muscle_group"] = self.muscle_group
        return data


class WorkoutFactory:
    @staticmethod
    def from_dict(data: dict[str, Any]) -> Workout:
        status = WorkoutStatus[data["status"]]

        if data["type"] == "cardio":
            return CardioWorkout(data["id"], data["title"], data["minutes"], data["intensity"], status)

        if data["type"] == "strength":
            return StrengthWorkout(data["id"], data["title"], data["minutes"], data["muscle_group"], status)

        raise FitnessValidationError("Неизвестная тренировка")


class WorkoutStorage:
    def __init__(self, filename: str = "fitness_workouts.json"):
        self.path = Path(filename)

    def load(self) -> list[Workout]:
        if not self.path.exists():
            return []

        with self.path.open("r", encoding="utf-8") as file:
            raw = json.load(file)

        return [WorkoutFactory.from_dict(item) for item in raw]

    def save(self, workouts: list[Workout]) -> None:
        with self.path.open("w", encoding="utf-8") as file:
            json.dump([workout.to_dict() for workout in workouts], file, ensure_ascii=False, indent=4)


class FitnessService:
    def __init__(self, storage: WorkoutStorage):
        self.storage = storage
        self.workouts = storage.load()

    def _next_id(self) -> int:
        return 1 if not self.workouts else max(workout.id for workout in self.workouts) + 1

    def add_cardio(self, title: str, minutes: int, intensity: int) -> None:
        self.workouts.append(CardioWorkout(self._next_id(), title, minutes, intensity))
        self.storage.save(self.workouts)

    def add_strength(self, title: str, minutes: int, group: str) -> None:
        self.workouts.append(StrengthWorkout(self._next_id(), title, minutes, group))
        self.storage.save(self.workouts)

    def find(self, workout_id: int) -> Workout:
        for workout in self.workouts:
            if workout.id == workout_id:
                return workout
        raise WorkoutNotFoundError("Тренировка не найдена")

    def done(self, workout_id: int) -> None:
        self.find(workout_id).mark_done()
        self.storage.save(self.workouts)

    def skipped(self, workout_id: int) -> None:
        self.find(workout_id).mark_skipped()
        self.storage.save(self.workouts)

    def delete(self, workout_id: int) -> None:
        workout = self.find(workout_id)
        self.workouts.remove(workout)
        self.storage.save(self.workouts)

    def total_calories(self) -> int:
        return sum(w.calories() for w in self.workouts if w.status == WorkoutStatus.DONE)


class FitnessApp:
    def __init__(self, page: ft.Page):
        self.page = page
        self.service = FitnessService(WorkoutStorage())

        self.workout_type = ft.Dropdown(
            label="Тип",
            value="Кардио",
            options=[ft.dropdown.Option("Кардио"), ft.dropdown.Option("Силовая")],
            width=150,
            on_change=self.type_changed,
        )
        self.title = ft.TextField(label="Название", width=230)
        self.minutes = ft.TextField(label="Минуты", width=120)
        self.extra = ft.TextField(label="Интенсивность", width=180)
        self.message = ft.Text("")
        self.summary = ft.Text("")
        self.items = ft.Column(scroll=ft.ScrollMode.AUTO, expand=True)

    def build(self):
        self.page.title = "Fitness Diary"
        self.page.window_width = 760
        self.page.window_height = 650

        self.page.add(
            ft.Column([
                ft.Text("Fitness Diary", size=26, weight=ft.FontWeight.BOLD),
                ft.Text("План тренировок, чуть простой но рабочий"),
                ft.Row([self.workout_type, self.title, self.minutes, self.extra], wrap=True),
                ft.Row([
                    ft.ElevatedButton("Добавить", on_click=self.add),
                    ft.OutlinedButton("Обновить", on_click=lambda e: self.refresh()),
                ]),
                self.message,
                self.summary,
                ft.Divider(),
                self.items,
            ], expand=True)
        )
        self.refresh()

    def type_changed(self, e):
        if self.workout_type.value == "Кардио":
            self.extra.label = "Интенсивность 1-10"
        else:
            self.extra.label = "Группа мышц"
        self.page.update()

    def add(self, e):
        try:
            if self.workout_type.value == "Кардио":
                self.service.add_cardio(self.title.value, int(self.minutes.value), int(self.extra.value))
            else:
                self.service.add_strength(self.title.value, int(self.minutes.value), self.extra.value)

            self.title.value = ""
            self.minutes.value = ""
            self.extra.value = ""
            self.message.value = "Тренировка добавлена"
            self.refresh()
        except Exception as error:
            self.message.value = f"Ошибка: {error}"
            self.page.update()

    def refresh(self):
        self.items.controls.clear()
        self.summary.value = f"Сожжено по выполненным: {self.service.total_calories()} ккал"

        if not self.service.workouts:
            self.items.controls.append(ft.Text("Тренировок пока нет"))
        else:
            for workout in self.service.workouts:
                self.items.controls.append(self.card(workout))

        self.page.update()

    def card(self, workout: Workout):
        return ft.Container(
            padding=10,
            margin=ft.margin.only(bottom=8),
            border_radius=10,
            bgcolor=ft.Colors.GREEN_50,
            content=ft.Column([
                ft.Text(f"#{workout.id} {workout.short_info()}"),
                ft.Row([
                    ft.TextButton("Выполнено", on_click=lambda e, i=workout.id: self.do_done(i)),
                    ft.TextButton("Пропущено", on_click=lambda e, i=workout.id: self.do_skip(i)),
                    ft.TextButton("Удалить", on_click=lambda e, i=workout.id: self.do_delete(i)),
                ])
            ])
        )

    def do_done(self, workout_id: int):
        self.service.done(workout_id)
        self.refresh()

    def do_skip(self, workout_id: int):
        self.service.skipped(workout_id)
        self.refresh()

    def do_delete(self, workout_id: int):
        self.service.delete(workout_id)
        self.refresh()


def main(page: ft.Page):
    FitnessApp(page).build()


if __name__ == "__main__":
    if hasattr(ft, "run"):
        ft.run(main)
    else:
        ft.app(target=main)
