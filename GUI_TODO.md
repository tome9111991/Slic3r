# GUI Framework Roadmap & TODO

Dieses Dokument dient als Roadmap für die Entwicklung unseres eigenen, systemunabhängigen UI-Frameworks für Slic3r.
Ziel: Volle Kontrolle über Design (Dark/Light Mode) und UX unter Windows, ohne native Einschränkungen.

## 🏗 Phase 1: Das Fundament (Core Infrastructure)

Hier geht es um die Basisklassen und das Management von Ressourcen.

- [x] **ThemeManager Basic:** Verwaltung von Farben und Dark-Mode Toggle.
- [x] **CanvasTheme Separation:** Trennung von UI- und 3D-Farben (`Legacy ColorScheme` entfernt).
- [x] **SVG Support:** Automatisches Laden von Icons je nach Theme.
- [x] **Font Manager:**
    - Zentrale Verwaltung von Schriftarten (Größen, Gewichte).
    - Anstatt `GetFont()` überall manuell zu setzen -> `ThemeManager::GetFont(FontSize::Small, FontWeight::Bold)`.
- [x] **HiDPI Scaling:**
    - Sicherstellen, dass alle `DrawRoundedRectangle` und SVGs auf 4K-Monitoren skalieren (Multiplikator aus `wxWindow::GetContentScaleFactor()` nutzen).
- [x] **State Persistence:**
    - Speichern der User-Präferenz (Dark/Light) in der `slic3r.ini`, damit die App beim Neustart richtig aussieht.

## 🧩 Phase 2: Essentielle Controls (Die Bausteine)

Wir müssen native Windows-Controls ersetzen oder "wrappen", da diese sich oft nicht umfärben lassen.

- [x] **ThemedCheckBox:**
    - Ersetzt `wxCheckBox`. Nutzt `tick.svg` mit dynamischem Recoloring.
- [x] **ThemedTextInput (Eingabefelder):**
    - *Problem:* `wxTextCtrl` lässt sich schwer stylen (Rahmenfarbe).
    - *Lösung:* Ein `wxPanel` als Container zeichnen (unser Border & Background), darin ein rahmenloses natives `wxTextCtrl` platzieren.
- [x] **ThemedNumberInput (Spinner):**
    - Wichtig für Slic3r (Layerhöhe, Temperaturen).
    - Kombination aus `ThemedTextInput` und zwei kleinen `ThemedButtons` (Up/Down).
- [ ] **ThemedSlider:**
    - Komplett selbst gezeichneter Slider (Track, Handle, Value-Tooltip).
    - Native Slider sehen im Dark Mode unter Windows oft schlecht aus.
- [ ] **ThemedProgressBar:**
    - Schlanker, moderner Ladebalken für das Slicing.

## 📦 Phase 3: Layout & Container

- [ ] **ThemedTabControl (Notebook Ersatz):**
    - Native Tabs sind unter Windows weiß und hässlich im Dark Mode.
    - Eigene Tab-Leiste (Buttons) + Panel-Switching-Logik.
- [ ] **ThemedScrollPane (Scrolling):**
    - *Der Endgegner:* Windows Scrollbars sind grau und breit.
    - Entweder `wxOverlay` nutzen um eigene Scrollbars über den Content zu malen, oder (einfacher) das Design der Scrollbars akzeptieren, aber den Hintergrund anpassen.
- [ ] **ThemedGroupBox / Separator:**
    - Visuelle Trenner für Einstellungs-Gruppen.

## 🎨 Phase 4: Fenster & Dialoge

- [ ] **ThemedDialog Base Class:**
    - Basisklasse für modale Fenster.
    - *Herausforderung:* Die "Title Bar" (Fensterleiste oben) ist vom Betriebssystem gesteuert.
    - *Lösung:* Entweder akzeptieren (einfach) oder `wxFRAME_NO_TASKBAR` nutzen und eine eigene Title-Bar zeichnen (aufwendig, aber perfekt für Dark Mode).
- [ ] **Toast Notifications:**
    - Kleine Popups ("Gespeichert", "Export fertig"), die sich nicht in den Vordergrund drängen.

## 🛠 Phase 5: Developer Experience & Testing

- [x] **Widget Gallery (Showcase):**
    - Ein separates Fenster (nur für Devs), in dem alle Controls untereinander angezeigt werden.
    - Dient zum Testen von Änderungen am Theme, ohne die ganze App bedienen zu müssen.
    - Wie "Storybook" in der Webentwicklung.

## 📂 Refactoring (Laufend)

- [ ] Sobald `ThemedControls.cpp` zu groß wird -> Aufsplitten in `src/GUI/Widgets/Button.cpp`, `src/GUI/Widgets/Input.cpp` etc.
- [ ] Namenskonventionen finalisieren (Namespace `Slic3r::GUI::UI`?).
