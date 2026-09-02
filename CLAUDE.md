# Qanvas

Runtime Qt6/QML (C++20, CMake, SQLite; desktop + Android). Ładuje aplikacje `.qml` z dysku i uruchamia bez kompilacji i restartu — każda dostaje własny kontekst QML i własny plik SQLite.

Zamysł: aplikacje pisze agent LLM na żądanie użytkownika. Zarys w [doc/idea.md](doc/idea.md) — na razie tylko kierunek, architektura otwarta. W repo działa szkielet: `AppLoader` + `AppContext` (`src/runtime/`) i testowa aplikacja w `test/`.

## Zasady

- **Nie dodawaj `Co-Authored-By` ani żadnej innej atrybucji do commitów i PR-ów.**
- **Bez komentarzy w kodzie**: nie dodawaj komentarzy (C++/QML); kod ma być samoopisujący się.

## Komponenty QML

- **Małe komponenty, składane w większe.** Jeden komponent = jedna rzecz (bąbelek, kropki „pisze”, lista, pole odpowiedzi); strona tylko je zestawia.
- **Style i rozmiary wyłącznie w `Theme`** (`src/qml/Theme.qml`). W komponencie nie ma kolorów, rozmiarów, promieni ani odstępów wpisanych na sztywno.
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
- **Najpierw `libs`.** `Themed.Components` i `App.Components` (`ThemedPage`, `ThemedButton`, `ThemedInput`, `ThemedIcon`, `ThemedChat`, …) — własny komponent piszemy dopiero, gdy w libs nie ma odpowiednika. **Libs nie modyfikujemy** pod potrzeby aplikacji.
- **Ikony trzymamy w `libs/icons`**, nie w aplikacji. Nie twórz nowej ikony, jeśli w libs już jest pasująca.
- **ViewModel podpinamy tylko do stron.** Pozostałe komponenty nie znają modelu — dostają dane propercjami i oddają zdarzenia sygnałami.
