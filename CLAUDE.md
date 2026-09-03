# Qanvas

Runtime Qt6/QML (C++20, CMake, SQLite; desktop + Android). Ładuje aplikacje `.qml` z dysku i uruchamia bez kompilacji i restartu — każda dostaje własny kontekst QML i własny plik SQLite.

Zamysł: aplikacje pisze agent LLM na żądanie użytkownika. Zarys w [doc/idea.md](doc/idea.md) — na razie tylko kierunek, architektura otwarta. W repo działa szkielet: `AppLoader` + `AppContext` (`src/runtime/`) i testowa aplikacja w `test/`.

## Zasady

- **Nie dodawaj `Co-Authored-By` ani żadnej innej atrybucji do commitów i PR-ów.**
- **Pliki robocze tylko w `tmp/` w repo** (ignorowany przez git) — skrypty pomocnicze, dane testowe, zrzuty. Nic poza katalogiem repo.

## Układ `src/runtime`

- `app/` — aplikacja na dysku: `App` (id, katalog, ścieżki), `AppFiles`, `AppRegistry` (lista apek i uprawnienia w `system.db`), `AppSchema` (tabele w `app.db` apki, tylko do odczytu), `AppCompiler`, `AppWarnings`.
- `storage/` — store'y na `system.db`: `SystemDatabase`, `ChatStore`, `NotesStore`, `TasksStore`.
- `host/` — to, co widzi działająca apka: `AppLoader`, `AppContext` (`App` w QML), `StoreBridge` i mostki `NotesBridge`/`TasksBridge` (`Notes`/`Tasks` w QML, za uprawnieniem z `AppRegistry`).
- `chat/` — strona agenta: `ChatAgent`, `AppTools` (toole), `ChatThread`.

Nowy współdzielony store = klasa w `storage/`, mostek dziedziczący po `StoreBridge` w `host/`, wpis w `sharedStores()` w `apptools.cpp`, akapit w prompcie systemowym i testy.

## Komentarze

Zero komentarzy w plikach `.cpp`, `.h` i `.qml` — w `src/`, `libs/` i `test/`.

- Zakaz obejmuje każdą formę: `//`, `/* */`, bloki nagłówkowe nad plikiem, klasą, funkcją i propercją, `TODO`/`FIXME`, separatory sekcji, notatki o wersjach Qt i o obejściach bugów.
- Nie ma wyjątku „to jest *dlaczego*, a nie *co*”. Nie ma wyjątku dla opisu API ani dla wiedzy, której nie widać z kodu.
- Powód nieoczywistej decyzji zapisz w commit message albo w `doc/` — nigdy w pliku źródłowym.
- Komentarze zastane w repo nie są wzorem stylu. Nie naśladuj ich gęstości w nowym kodzie.
- Z fragmentów, które i tak zmieniasz, komentarze usuwaj.

## Komponenty QML

- **Małe komponenty, składane w większe.** Jeden komponent = jedna rzecz (bąbelek, kropki „pisze”, lista, pole odpowiedzi); strona tylko je zestawia.
- **Style i rozmiary wyłącznie w `Theme`** (`src/components/Theme.qml`). W komponencie nie ma kolorów, rozmiarów, promieni ani odstępów wpisanych na sztywno.
- **Minimalne API.** Wariant wybiera się nazwą, nie zestawem propercji — 3–4 propercje mają wystarczyć:

  ```qml
  ThemedButton {
      text: "Button"
      buttonSize: Theme.button.small
      buttonStyle: Theme.button.primary
      onClicked: ...
  }
  ```

  Obiekt stylu w `Theme` niesie komplet (tło, obramowanie, kolor tekstu, padding, wyrównanie), więc komponent nie rozgałęzia się po wariantach.
- **Najpierw `libs`.** `Themed.Components` i `App.Components` (`ThemedPage`, `ThemedButton`, `ThemedInput`, `ThemedIcon`, `ThemedChat`, …) — własny komponent piszemy dopiero, gdy w libs nie ma odpowiednika. Brakującą rzecz ogólnego użytku dokładamy do `Themed.Components`; libs nie naginamy pod jedną aplikację.
- **Ikony trzymamy w `libs/icons`**, nie w aplikacji. Nie twórz nowej ikony, jeśli w libs już jest pasująca.
- **ViewModel podpinamy tylko do stron.** Pozostałe komponenty nie znają modelu — dostają dane propercjami i oddają zdarzenia sygnałami.

## Moduły QML

- `Qanvas` — runtime (typy z C++) i powłoka hosta: `Main`, `MainView`, ekran czatu, zakładka aplikacji (`src/qml/`).
- `Qanvas.Components` — `Theme` i komponenty `Qanvas*` dla aplikacji pisanych przez agenta (`src/components/`).

Komponenty `Qanvas*` to cienkie nakładki na `Themed*`: wystawiają wyłącznie to, co agent musi ustawić (dane, zdarzenia, `type`), bez żadnych propercji stylu. Wariant wybiera się słowami w `type` (`type: "danger small"`), a mapowanie `type` → styl siedzi w `Theme` (`buttonStyle`, `buttonSize`, `textStyle`). Czego brakuje, dopisujemy najpierw do `Themed.Components` w libs, a `Qanvas*` z tego dziedziczy.
