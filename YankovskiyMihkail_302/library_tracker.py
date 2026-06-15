from __future__ import annotations

from abc import ABC, abstractmethod
from enum import Enum
import json
from pathlib import Path
from typing import Any

import flet as ft


class BookStatus(Enum):
    PLANNED = "хочу прочитать"
    READING = "читаю"
    DONE = "прочитано"


class BookError(Exception):
    pass


class BookValidationError(BookError):
    pass


class BookNotFoundError(BookError):
    pass


class Book(ABC):
    book_type = "base"

    def __init__(self, book_id: int, title: str, author: str, status: BookStatus = BookStatus.PLANNED):
        self.id = book_id
        self._title = ""
        self.title = title
        self.author = author.strip() or "Автор не указан"
        self.status = status

    @property
    def title(self) -> str:
        return self._title

    @title.setter
    def title(self, value: str) -> None:
        if not value or not value.strip():
            raise BookValidationError("Название книги не может быть пустым")
        self._title = value.strip()

    def mark_reading(self) -> None:
        self.status = BookStatus.READING

    def mark_done(self) -> None:
        self.status = BookStatus.DONE

    @abstractmethod
    def short_info(self) -> str:
        pass

    def to_dict(self) -> dict[str, Any]:
        return {
            "type": self.book_type,
            "id": self.id,
            "title": self.title,
            "author": self.author,
            "status": self.status.name,
        }


class PaperBook(Book):
    book_type = "paper"

    def __init__(self, book_id: int, title: str, author: str, pages: int, status: BookStatus = BookStatus.PLANNED):
        super().__init__(book_id, title, author, status)
        self.pages = int(pages)

    def short_info(self) -> str:
        return f"Бумажная: {self.title} — {self.author}, {self.pages} стр. | {self.status.value}"

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["pages"] = self.pages
        return data


class AudioBook(Book):
    book_type = "audio"

    def __init__(self, book_id: int, title: str, author: str, minutes: int, status: BookStatus = BookStatus.PLANNED):
        super().__init__(book_id, title, author, status)
        self.minutes = int(minutes)

    def short_info(self) -> str:
        return f"Аудио: {self.title} — {self.author}, {self.minutes} мин. | {self.status.value}"

    def to_dict(self) -> dict[str, Any]:
        data = super().to_dict()
        data["minutes"] = self.minutes
        return data


class BookFactory:
    @staticmethod
    def from_dict(data: dict[str, Any]) -> Book:
        status = BookStatus[data["status"]]

        if data["type"] == "paper":
            return PaperBook(data["id"], data["title"], data["author"], data["pages"], status)

        if data["type"] == "audio":
            return AudioBook(data["id"], data["title"], data["author"], data["minutes"], status)

        raise BookValidationError("Неизвестный тип книги")


class JsonBookStorage:
    def __init__(self, filename: str = "library_books.json"):
        self.path = Path(filename)

    def load(self) -> list[Book]:
        if not self.path.exists():
            return []

        with self.path.open("r", encoding="utf-8") as file:
            raw = json.load(file)

        return [BookFactory.from_dict(item) for item in raw]

    def save(self, books: list[Book]) -> None:
        with self.path.open("w", encoding="utf-8") as file:
            json.dump([book.to_dict() for book in books], file, ensure_ascii=False, indent=4)


class LibraryService:
    def __init__(self, storage: JsonBookStorage):
        self.storage = storage
        self.books = storage.load()

    def _next_id(self) -> int:
        return 1 if not self.books else max(book.id for book in self.books) + 1

    def add_paper_book(self, title: str, author: str, pages: int) -> None:
        self.books.append(PaperBook(self._next_id(), title, author, pages))
        self.storage.save(self.books)

    def add_audio_book(self, title: str, author: str, minutes: int) -> None:
        self.books.append(AudioBook(self._next_id(), title, author, minutes))
        self.storage.save(self.books)

    def find(self, book_id: int) -> Book:
        for book in self.books:
            if book.id == book_id:
                return book
        raise BookNotFoundError("Книга не найдена")

    def set_reading(self, book_id: int) -> None:
        self.find(book_id).mark_reading()
        self.storage.save(self.books)

    def set_done(self, book_id: int) -> None:
        self.find(book_id).mark_done()
        self.storage.save(self.books)

    def delete(self, book_id: int) -> None:
        book = self.find(book_id)
        self.books.remove(book)
        self.storage.save(self.books)


class LibraryApp:
    def __init__(self, page: ft.Page):
        self.page = page
        self.service = LibraryService(JsonBookStorage())

        self.book_type = ft.Dropdown(
            label="Тип",
            value="Бумажная",
            options=[ft.dropdown.Option("Бумажная"), ft.dropdown.Option("Аудио")],
            width=150,
        )
        self.title = ft.TextField(label="Название", width=230)
        self.author = ft.TextField(label="Автор", width=220)
        self.number = ft.TextField(label="Страницы / минуты", width=180)
        self.message = ft.Text("")
        self.list_view = ft.Column(scroll=ft.ScrollMode.AUTO, expand=True)

    def build(self):
        self.page.title = "Library Tracker"
        self.page.window_width = 760
        self.page.window_height = 650

        self.page.add(
            ft.Column(
                [
                    ft.Text("Library Tracker", size=26, weight=ft.FontWeight.BOLD),
                    ft.Text("Мини учет книг на Flet, тут ООП + JSON"),
                    ft.Row([self.book_type, self.title, self.author, self.number], wrap=True),
                    ft.Row([
                        ft.ElevatedButton("Добавить", on_click=self.add_book),
                        ft.OutlinedButton("Обновить", on_click=lambda e: self.refresh()),
                    ]),
                    self.message,
                    ft.Divider(),
                    self.list_view,
                ],
                expand=True,
            )
        )
        self.refresh()

    def add_book(self, e):
        try:
            num = int(self.number.value)
            if self.book_type.value == "Бумажная":
                self.service.add_paper_book(self.title.value, self.author.value, num)
            else:
                self.service.add_audio_book(self.title.value, self.author.value, num)

            self.title.value = ""
            self.author.value = ""
            self.number.value = ""
            self.message.value = "Книга добавлена"
            self.refresh()
        except Exception as error:
            self.message.value = f"Ошибка: {error}"
            self.page.update()

    def refresh(self):
        self.list_view.controls.clear()

        if not self.service.books:
            self.list_view.controls.append(ft.Text("Книг пока нету"))
        else:
            for book in self.service.books:
                self.list_view.controls.append(self.card(book))

        self.page.update()

    def card(self, book: Book):
        return ft.Container(
            padding=10,
            margin=ft.margin.only(bottom=8),
            border_radius=10,
            bgcolor=ft.Colors.BLUE_50,
            content=ft.Column([
                ft.Text(f"#{book.id} {book.short_info()}"),
                ft.Row([
                    ft.TextButton("Читаю", on_click=lambda e, i=book.id: self.do_reading(i)),
                    ft.TextButton("Готово", on_click=lambda e, i=book.id: self.do_done(i)),
                    ft.TextButton("Удалить", on_click=lambda e, i=book.id: self.do_delete(i)),
                ])
            ])
        )

    def do_reading(self, book_id: int):
        self.service.set_reading(book_id)
        self.refresh()

    def do_done(self, book_id: int):
        self.service.set_done(book_id)
        self.refresh()

    def do_delete(self, book_id: int):
        self.service.delete(book_id)
        self.refresh()


def main(page: ft.Page):
    LibraryApp(page).build()


if __name__ == "__main__":
    if hasattr(ft, "run"):
        ft.run(main)
    else:
        ft.app(target=main)
