# RmlUI Integration Guide

## Overview

This project uses [RmlUI](https://github.com/mikke89/RmlUi) for HTML/CSS-based user interfaces. RmlUI allows you to design UIs using familiar web technologies while rendering them natively with OpenGL through raylib.

## Project Structure

```
assets/ui/rml/
├── README.md           # This file
├── mainmenu.rml        # Main menu UI document
├── mainmenu.css        # Main menu styles
├── hud.rml            # In-game HUD (not yet implemented)
├── hud.css            # HUD styles
└── fonts/             # Font files
    ├── LatoLatin-Regular.ttf
    ├── LatoLatin-Bold.ttf
    └── UbuntuMono-Regular.ttf

src/ui/rmlui/
├── raylibRmlUi.h              # Main RmlUI wrapper interface
├── raylibRmlUi.cpp            # RmlUI initialization and management
├── raylibRenderInterface.h/cpp # Rendering backend (OpenGL via raylib)
├── raylibSystemInterface.h/cpp # Input handling (keyboard/mouse)
├── raylibFileInterface.h/cpp   # File loading
└── GameEventListener.h         # Event handler for button clicks
```

## How RmlUI Works

1. **Documents (.rml)**: HTML-like markup files that define UI structure
2. **Stylesheets (.css)**: CSS files that define appearance and layout
3. **Fonts**: TTF fonts loaded before documents
4. **Event Listeners**: C++ callbacks attached to UI elements

## Creating a New UI Document

### 1. Create the RML File

Create a new `.rml` file in `assets/ui/rml/`:

```xml
<rml>
<head>
    <title>My UI</title>
    <link type="text/css" href="myui.css"/>
</head>
<body>
    <div id="container">
        <h1>Hello RmlUI!</h1>
        <button id="my-button">Click Me</button>
    </div>
</body>
</rml>
```

### 2. Create the CSS File

Create a matching `.css` file:

```css
body {
    font-family: LatoLatin;
    color: #ffffff;
    width: 100%;
    height: 100%;
}

#container {
    position: absolute;
    left: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
    width: 400px;
}

button {
    padding: 16px 24px;
    background: #2a3854;
    color: #ffffff;
    font-size: 18px;
    border-radius: 8px;
    cursor: pointer;
}

button:hover {
    background: #3d5070;
}
```

### 3. Load the Document in C++

In `Game.cpp` initialization:

```cpp
// Load the document
RaylibRmlUi::LoadRml("assets/ui/rml/myui.rml", "myui", false);
Rml::ElementDocument* myDoc = RaylibRmlUi::GetPage("myui");

// Setup event handlers
if (myDoc)
{
    auto button = myDoc->GetElementById("my-button");
    if (button)
    {
        button->AddEventListener(Rml::EventId::Click, new GameEventListener([this](Rml::Event&) {
            // Handle button click
            TraceLog(LOG_INFO, "Button clicked!");
        }));
    }
    
    // Show the document
    myDoc->Show();
}
```

### 4. Update Visibility

Control when the UI is visible:

```cpp
// Show/hide documents
if (myDoc && !myDoc->IsVisible())
    myDoc->Show();

if (myDoc && myDoc->IsVisible())
    myDoc->Hide();
```

## Supported CSS Properties

RmlUI supports a subset of CSS. **Important limitations:**

### ✅ Supported
- Colors: hex (`#ffffff`), rgb (`rgb(255, 255, 255)`)
- Layout: `display`, `position`, `width`, `height`, `margin`, `padding`
- Flexbox: `display: flex`, `gap`, `justify-content`, `align-items`
- Text: `font-family`, `font-size`, `font-weight`, `color`, `letter-spacing`
- Background: solid colors only (`background: #1a2033;`)
- Border: `border-radius` (no border colors with alpha in current setup)
- Transform: `transform: translate()`, `translateX()`, `translateY()`
- Transitions: basic `transition` support

### ❌ NOT Supported
- `linear-gradient()` - use solid colors instead
- `rgba()` with alpha in border/shadow - use hex without alpha
- `text-shadow` - not supported
- `box-shadow` - limited support
- CSS Grid - use flexbox instead
- Complex selectors - keep selectors simple

### Workarounds

**Gradients** → Use solid color approximation:
```css
/* Instead of: background: linear-gradient(135deg, #1f2e4a, #2d3f5e); */
background: #263650; /* Middle tone */
```

**Transparent borders** → Omit border or use solid color:
```css
/* Instead of: border: 1px solid rgba(255,255,255,0.1); */
/* Just omit it or use: */
border: 1px #444444;
```

## Event Handling

### Available Events
- `Rml::EventId::Click` - Mouse click
- `Rml::EventId::Dblclick` - Double click
- `Rml::EventId::Mousedown` - Mouse button pressed
- `Rml::EventId::Mouseup` - Mouse button released
- `Rml::EventId::Mouseover` - Mouse enters element
- `Rml::EventId::Mouseout` - Mouse leaves element
- `Rml::EventId::Focus` - Element gains focus
- `Rml::EventId::Blur` - Element loses focus
- `Rml::EventId::Keydown` - Key pressed
- `Rml::EventId::Keyup` - Key released
- `Rml::EventId::Submit` - Form submitted

### Example Event Listener

```cpp
auto element = document->GetElementById("my-element");
if (element)
{
    element->AddEventListener(Rml::EventId::Click, 
        new GameEventListener([this](Rml::Event& event) {
            // Access the element that triggered the event
            Rml::Element* target = event.GetTargetElement();
            
            // Modify the element
            target->SetInnerRML("Clicked!");
            
            // Call game logic
            this->DoSomething();
        })
    );
}
```

## Updating Dynamic Content

```cpp
// Get element by ID
if (Rml::Element* elem = document->GetElementById("score"))
{
    // Update text content
    elem->SetInnerRML(TextFormat("Score: %d", playerScore));
}

// Update attributes
if (Rml::Element* button = document->GetElementById("submit"))
{
    button->SetAttribute("disabled", "");  // Disable button
    button->RemoveAttribute("disabled");    // Enable button
}

// Update styles
if (Rml::Element* bar = document->GetElementById("health-bar"))
{
    bar->SetProperty("width", TextFormat("%dpx", healthPercent * 2));
    bar->SetProperty("background", "#ff0000");
}
```

## Font Management

Fonts must be loaded **before** loading any RML documents:

```cpp
// Load fonts first
RaylibRmlUi::LoadFontFace("assets/ui/rml/fonts/LatoLatin-Regular.ttf", "Lato", false);
RaylibRmlUi::LoadFontFace("assets/ui/rml/fonts/LatoLatin-Bold.ttf", "Lato Bold", false);

// Then load documents that use those fonts
RaylibRmlUi::LoadRml("assets/ui/rml/mainmenu.rml", "mainmenu", false);
```

**Important:** The font family name comes from the TTF file metadata, not the filename. Use the actual family name in CSS:

```css
/* The TTF file contains "LatoLatin" as family name */
body {
    font-family: LatoLatin;  /* Correct */
    /* font-family: "Lato"; */ /* Wrong - will not work */
}
```

## Layout Tips

### Centering

```css
/* Center with absolute positioning */
#centered {
    position: absolute;
    left: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
}

/* Center with flex */
.flex-container {
    display: flex;
    justify-content: center;
    align-items: center;
    width: 100%;
    height: 100%;
}
```

### Responsive Sizing

```css
/* Use percentages */
#responsive {
    width: 80%;
    max-width: 800px;
}

/* Use viewport units */
body {
    width: 100%;
    height: 100%;
}
```

### Flexbox Layout

```css
.menu-buttons {
    display: flex;
    flex-direction: column;
    gap: 16px;
}

.horizontal-bar {
    display: flex;
    gap: 12px;
    justify-content: space-between;
}
```

## Common Issues

### Font Not Loading
**Problem:** "No font face defined" warning
**Solution:** 
1. Check font family name in CSS matches the TTF internal name
2. Ensure fonts are loaded before RML documents
3. Don't use fallback fonts like `, sans-serif`

### Element Not Clickable
**Problem:** Clicks don't register
**Solution:**
1. Check `cursor: pointer;` is set in CSS
2. Ensure element is not covered by another element
3. Verify document is shown: `document->Show()`

### CSS Not Applied
**Problem:** Styles don't appear
**Solution:**
1. Check CSS file path in RML `<link>` tag is correct
2. Verify selectors match element IDs/classes
3. Remove unsupported properties (gradients, rgba borders, etc.)

### UI Not Visible
**Problem:** Nothing appears on screen
**Solution:**
1. Call `RaylibRmlUi::Update()` every frame
2. Call `RaylibRmlUi::Draw()` every frame
3. Ensure document is shown: `document->Show()`
4. Check document isn't hidden behind game scene

## Example: Main Menu (Current Implementation)

See `mainmenu.rml` and `mainmenu.css` for a complete working example of:
- Centered layout with transform
- Button styling with hover effects
- Event handling for navigation
- Proper font usage

## Debugging

### Enable RmlUI Debugger

Press **F8** in-game to toggle the RmlUI debugger, which shows:
- Element hierarchy
- Applied styles
- Computed layout
- Event listeners

### Console Logs

RmlUI will log warnings about:
- Missing fonts
- Invalid CSS properties
- File loading errors

Check the console output for `[RmlUi]` messages.

## Resources

- [RmlUI Documentation](https://mikke89.github.io/RmlUiDoc/)
- [RmlUI GitHub](https://github.com/mikke89/RmlUi)
- [RmlUI CSS Support](https://mikke89.github.io/RmlUiDoc/pages/rcss.html)
- [RmlUI Element Reference](https://mikke89.github.io/RmlUiDoc/pages/rml.html)
