from __future__ import annotations

from abc import ABC, abstractmethod
from enum import Enum
import json
from pathlib import Path
from typing import Any

import flet as ft


class Priority(Enum):
    LOW = "низкий"
    MEDIUM = "средний"
    HIGH = "высокий"


class TaskStatus(Enum):
    TODO = "надо сделать"
    PROCESS = "делается"
    DONE = "готово"


class TaskError(Exception):
    pass


class TaskValidationError(TaskError):
    pass


class TaskNotFoundError(TaskError):
    pass


class StudyTask(ABC):
    task_type = "base"

    def __init__(
        self,
        task_id: int,
        title: str,
        description: str,
        priority: Priority,
        status: TaskStatus = TaskStatus.TODO,
    ):
        self.id = task_id
        self._title = ""
        self.title = title
        self.description = description
        self.priority = priority
        self.status = status

    @property
    def title(self):
        return self._title

    @title.setter
    def title(self, value: str):
        if not value or not value.strip():
            raise TaskValidationError("Название не должно быть пустым")
        self._title = value.strip()

    def start(self):
        if self.status == TaskStatus.DONE:
            raise TaskValidationError("Готовую задачу уже нельзя начать")
        self.status = TaskStatus.PROCESS

    def finish(self):
        self.status = TaskStatus.DONE

    @abstractmethod
    def short_info(self) -> str:
        pass

    def to_dict(self) -> dict[str, Any]:
        return {
            "type": self.task_type,
            "id": self.id,
            "title": self.title,
            "description": self.description,
            "priority": self.priority.name,
            "status": self.status.name,
        }


class HomeworkTask(StudyTask):
    task_type = "homework"

    def __init__(
        self,
        task_id: int,
        title: str,
        description: str,
        priority: Priority,
        subject: str,
        status: TaskStatus = TaskStatus.TODO,
    ):
        super().__init__(task_id, title, description, priority, status)
        self.subject = subject.strip() or "предмет не указан"

    def short_info(self) -> str:
        return (
            f"Домашка по {self.subject}: {self.title}\n"
            f"Приоритет: {self.priority.value}, статус: {self.status.value}"
        )

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["subject"] = self.subject
        return data


class ExamTask(StudyTask):
    task_type = "exam"

    def __init__(
        self,
        task_id: int,
        title: str,
        description: str,
        priority: Priority,
        subject: str,
        date: str,
        status: TaskStatus = TaskStatus.TODO,
    ):
        super().__init__(task_id, title, description, priority, status)
        self.subject = subject.strip() or "предмет не указан"
        self.date = date.strip() or "дата не указана"

    def short_info(self) -> str:
        return (
            f"Экзамен: {self.subject}\n"
            f"{self.title}, дата: {self.date}\n"
            f"Приоритет: {self.priority.value}, статус: {self.status.value}"
        )

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["subject"] = self.subject
        data["date"] = self.date
        return data


class ProjectTask(StudyTask):
    task_type = "project"

    def __init__(
        self,
        task_id: int,
        title: str,
        description: str,
        priority: Priority,
        team: str,
        deadline: str,
        status: TaskStatus = TaskStatus.TODO,
    ):
        super().__init__(task_id, title, description, priority, status)
        self.team = team.strip() or "без команды"
        self.deadline = deadline.strip() or "без дедлайна"

    def short_info(self) -> str:
        return (
            f"Проект: {self.title}\n"
            f"Команда: {self.team}, дедлайн: {self.deadline}\n"
            f"Приоритет: {self.priority.value}, статус: {self.status.value}"
        )

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["team"] = self.team
        data["deadline"] = self.deadline
        return data


class TaskFactory:
    @staticmethod
    def from_dict(data: dict[str, Any]) -> StudyTask:
        priority = Priority[data["priority"]]
        status = TaskStatus[data["status"]]

        if data["type"] == "homework":
            return HomeworkTask(
                data["id"],
                data["title"],
                data["description"],
                priority,
                data["subject"],
                status,
            )

        if data["type"] == "exam":
            return ExamTask(
                data["id"],
                data["title"],
                data["description"],
                priority,
                data["subject"],
                data["date"],
                status,
            )

        if data["type"] == "project":
            return ProjectTask(
                data["id"],
                data["title"],
                data["description"],
                priority,
                data["team"],
                data["deadline"],
                status,
            )

        raise TaskValidationError("Неизвестный тип задачи")


class TaskStorage:
    def __init__(self, filename: str = "study_tasks.json"):
        self.path = Path(filename)

    def load(self) -> list[StudyTask]:
        if not self.path.exists():
            return []

        with self.path.open("r", encoding="utf-8") as file:
            data = json.load(file)

        return [TaskFactory.from_dict(item) for item in data]

    def save(self, tasks: list[StudyTask]) -> None:
        with self.path.open("w", encoding="utf-8") as file:
            json.dump([task.to_dict() for task in tasks], file, ensure_ascii=False, indent=4)


class TaskService:
    def __init__(self, storage: TaskStorage):
        self.storage = storage
        self.tasks = storage.load()

    def _next_id(self) -> int:
        if not self.tasks:
            return 1
        return max(task.id for task in self.tasks) + 1

    def add_task(
        self,
        task_type: str,
        title: str,
        description: str,
        priority: Priority,
        field1: str,
        field2: str,
    ):
        new_id = self._next_id()

        if task_type == "Домашка":
            task = HomeworkTask(new_id, title, description, priority, field1)

        elif task_type == "Экзамен":
            task = ExamTask(new_id, title, description, priority, field1, field2)

        elif task_type == "Проект":
            task = ProjectTask(new_id, title, description, priority, field1, field2)

        else:
            raise TaskValidationError("Неправильный тип задачи")

        self.tasks.append(task)
        self.storage.save(self.tasks)

    def find(self, task_id: int) -> StudyTask:
        for task in self.tasks:
            if task.id == task_id:
                return task
        raise TaskNotFoundError("Задача не найдена")

    def start(self, task_id: int):
        self.find(task_id).start()
        self.storage.save(self.tasks)

    def finish(self, task_id: int):
        self.find(task_id).finish()
        self.storage.save(self.tasks)

    def delete(self, task_id: int):
        task = self.find(task_id)
        self.tasks.remove(task)
        self.storage.save(self.tasks)


class StudyApp:
    def __init__(self, page: ft.Page):
        self.page = page
        self.service = TaskService(TaskStorage())

        self.task_type = ft.Dropdown(
            label="Тип",
            value="Домашка",
            options=[
                ft.dropdown.Option("Домашка"),
                ft.dropdown.Option("Экзамен"),
                ft.dropdown.Option("Проект"),
            ],
            width=150,
            on_change=self.change_type,
        )

        self.priority = ft.Dropdown(
            label="Приоритет",
            value=Priority.MEDIUM.value,
            options=[ft.dropdown.Option(p.value) for p in Priority],
            width=150,
        )

        self.title = ft.TextField(label="Название", width=260)
        self.description = ft.TextField(label="Описание", width=270)
        self.field1 = ft.TextField(label="Предмет", width=200)
        self.field2 = ft.TextField(label="Дата / дедлайн", width=200)

        self.info = ft.Text("")
        self.task_column = ft.Column(scroll=ft.ScrollMode.AUTO, expand=True)

    def build(self):
        self.page.title = "Study Planner"
        self.page.window_width = 820
        self.page.window_height = 680

        self.page.add(
            ft.Column(
                [
                    ft.Text("Study Planner", size=28, weight=ft.FontWeight.BOLD),
                    ft.Text("Мини приложение для учебных задач"),
                    ft.Row([self.task_type, self.priority], wrap=True),
                    ft.Row([self.title, self.description], wrap=True),
                    ft.Row([self.field1, self.field2], wrap=True),
                    ft.Row(
                        [
                            ft.ElevatedButton("Добавить", on_click=self.add_task),
                            ft.OutlinedButton("Обновить", on_click=lambda e: self.refresh()),
                        ]
                    ),
                    self.info,
                    ft.Divider(),
                    self.task_column,
                ],
                expand=True,
            )
        )

        self.change_type(None)
        self.refresh()

    def get_priority(self) -> Priority:
        for p in Priority:
            if p.value == self.priority.value:
                return p
        return Priority.MEDIUM

    def change_type(self, e):
        if self.task_type.value == "Домашка":
            self.field1.label = "Предмет"
            self.field2.label = "Можно не заполнять"
            self.field2.disabled = True
            self.field2.value = ""

        elif self.task_type.value == "Экзамен":
            self.field1.label = "Предмет"
            self.field2.label = "Дата экзамена"
            self.field2.disabled = False

        else:
            self.field1.label = "Команда"
            self.field2.label = "Дедлайн"
            self.field2.disabled = False

        self.page.update()

    def add_task(self, e):
        try:
            self.service.add_task(
                self.task_type.value,
                self.title.value,
                self.description.value,
                self.get_priority(),
                self.field1.value or "",
                self.field2.value or "",
            )

            self.title.value = ""
            self.description.value = ""
            self.field1.value = ""
            self.field2.value = ""
            self.info.value = "Добавлено"
            self.info.color = ft.Colors.GREEN
            self.refresh()

        except Exception as error:
            self.info.value = f"Ошибка: {error}"
            self.info.color = ft.Colors.RED
            self.page.update()

    def refresh(self):
        self.task_column.controls.clear()

        if not self.service.tasks:
            self.task_column.controls.append(ft.Text("Задач пока нет"))
        else:
            for task in self.service.tasks:
                self.task_column.controls.append(self.make_card(task))

        self.page.update()

    def make_card(self, task: StudyTask):
        color = ft.Colors.BLUE_50
        if task.status == TaskStatus.PROCESS:
            color = ft.Colors.AMBER_100
        elif task.status == TaskStatus.DONE:
            color = ft.Colors.GREEN_100

        return ft.Container(
            padding=12,
            margin=ft.margin.only(bottom=8),
            border_radius=10,
            bgcolor=color,
            content=ft.Column(
                [
                    ft.Text(f"#{task.id}"),
                    ft.Text(task.short_info()),
                    ft.Text(task.description if task.description else "описания нет", size=12),
                    ft.Row(
                        [
                            ft.TextButton("Начать", on_click=lambda e, i=task.id: self.do_start(i)),
                            ft.TextButton("Готово", on_click=lambda e, i=task.id: self.do_finish(i)),
                            ft.TextButton("Удалить", on_click=lambda e, i=task.id: self.do_delete(i)),
                        ],
                        wrap=True,
                    ),
                ]
            ),
        )

    def do_start(self, task_id: int):
        try:
            self.service.start(task_id)
            self.info.value = "Статус изменен"
        except Exception as error:
            self.info.value = f"Ошибка: {error}"
        self.refresh()

    def do_finish(self, task_id: int):
        try:
            self.service.finish(task_id)
            self.info.value = "Задача закрыта"
        except Exception as error:
            self.info.value = f"Ошибка: {error}"
        self.refresh()

    def do_delete(self, task_id: int):
        try:
            self.service.delete(task_id)
            self.info.value = "Удалено"
        except Exception as error:
            self.info.value = f"Ошибка: {error}"
        self.refresh()


def main(page: ft.Page):
    app = StudyApp(page)
    app.build()


if __name__ == "__main__":
    if hasattr(ft, "run"):
        ft.run(main)
    else:
        ft.app(target=main)
