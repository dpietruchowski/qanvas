# Qanvas

Runtime Qt6/QML (C++20, CMake, SQLite; desktop + Android). Ładuje aplikacje `.qml` z dysku i uruchamia bez kompilacji i restartu — każda dostaje własny kontekst QML i własny plik SQLite.

Zamysł: aplikacje pisze agent LLM na żądanie użytkownika. Zarys w [doc/idea.md](doc/idea.md) — na razie tylko kierunek, architektura otwarta. W repo działa szkielet: `AppLoader` + `AppContext` (`src/runtime/`) i testowa aplikacja w `test/`.

## Zasady

- **Nie dodawaj `Co-Authored-By` ani żadnej innej atrybucji do commitów i PR-ów.**
- **Commit message: subject, pusta linia, jeden akapit najwyżej 5 linii.** Tyle. Bez drugiego akapitu i bez list — diff i testy mówią resztę.
- **Pliki robocze tylko w `tmp/` w repo** (ignorowany przez git) — skrypty pomocnicze, dane testowe, zrzuty. Nic poza katalogiem repo.

## Build i narzędzia

- Parametry aplikacji (nazwa, targety, wersja, pakiet Android, AppImage) siedzą w `app.env`; CMake czyta je przez `app_load_env()`, skrypty z `libs/scripts` przez `common.sh`. Wersji nie bumpuj ręcznie.
- Root `CMakeLists.txt` składa się z helperów z `libs/cmake/AppProject.cmake` (`app_add_module`, `app_add_qml_module`, `app_add_test`, `app_configure_android`). Pliki są globowane przy konfiguracji: po dodaniu albo przeniesieniu pliku `cmake -B build-desktop`.
- `libs/cpp` jest include rootem: `async/result.h`, `platform/securestorage.h`, `qmlutils/qmlregistrator.h`, `agent/agentcontroller.h`. Targety: `app_<katalog>`.
- Typy QML z runtime (`AppLoader`, `ChatAgent`, `AppRegistry`) rejestruje `main.cpp` przez `QmlRegistrator` — bez `QML_ELEMENT`, bo `qanvas_runtime` to zwykła biblioteka statyczna.
- `Theme.qml` dziedziczy po `DefaultTheme` z modułu `Themed.Theme` i **nie importuje `Themed.Components`** (cykl). Nadpisuje tylko to, co jest specyficzne dla qanvasa.
- Debug build: `cmake -B build-desktop -DCMAKE_BUILD_TYPE=Debug`, testy `ctest --test-dir build-desktop`, format `cmake --build build-desktop --target format_check`.
- Aplikację uruchamiaj przez `python3 libs/tools/ui_session.py start --platform offscreen` i steruj `python3 libs/tools/ui_driver.py …` (skill `ui-session`); log w `tmp/ui_session/app.log`. Android: `libs/scripts/build-android.sh debug apk`, AppImage: `libs/scripts/build-appimage.sh`.

## Układ `src/runtime`

- `app/` — aplikacja na dysku: `App` (id, katalog, ścieżki), `AppFiles`, `AppRegistry` (lista apek i uprawnienia w `system.db`), `AppSchema` (tabele w `app.db` apki, tylko do odczytu), `AppCompiler`, `AppWarnings`.
- `storage/` — store'y na `system.db`: `SystemDatabase`, `ChatStore`, `NotesStore`, `TasksStore`.
- `host/` — to, co widzi działająca apka: `AppLoader`, `AppContext` (`App` w QML), `StoreBridge` i mostki `NotesBridge`/`TasksBridge` (`Notes`/`Tasks` w QML, za uprawnieniem z `AppRegistry`), `AppLayout` (mierzy działającą apkę z `AppLoader::running(id)` i opisuje, co wyjeżdża poza ekran — wynik dokleja się do odpowiedzi każdego toola, który przeładowuje apkę, i do `check_layout`).
- `chat/` — strona agenta: `ChatAgent`, `AppTools` (toole), `ChatThread`, prompt systemowy w `systemprompt.txt` (zasób `:/qanvas/systemprompt.txt`, ładowany przez `SystemPrompt::text()`).

Prompt jest maszyną stanów (ROUTE → DESIGN → BUILD, INSPECT → MODIFY, FIX, DEBUG, DONE) z przykładami; przykładowe pliki i edycje z promptu muszą naprawdę się ładować — `python3 test/prompt_example.py` wyciąga je z promptu i uruchamia offscreen (debug build w `build-desktop`). Nowy tool albo komponent = wpis w prompcie i w `systemprompttest.cpp`.

Agenta testuj na żywo przez Claude CLI z modelem haiku (najsłabszy, więc najszybciej pokazuje luki w prompcie): `python3 tmp/agent_run.py <nazwa> haiku "prompt 1" "prompt 2"` uruchamia apkę offscreen, wysyła kolejne wiadomości i zapisuje do `tmp/agent-<nazwa>/` log, zrzuty ekranu i pliki apki; w wyjściu widać każdy tool call i jego wynik (`Tool result`). Po zmianie w prompcie, toolach albo komponentach przepuść stoper, quiz wieloekranowy i apkę z kafelkami i porównaj czas tury oraz liczbę nieudanych tooli.

Nowy współdzielony store = klasa w `storage/`, mostek dziedziczący po `StoreBridge` w `host/`, wpis w `sharedStores()` w `apptools.cpp`, akapit SHARED STORES w `systemprompt.txt` i testy.

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
