# Appstore listing

Copy-paste text and assets for the Pebble Developer Dashboard.
Requirements: <https://developer.repebble.com/guides/appstore-publishing/preparing-a-submission>

## App name

Scribbly Note

## Category

Tools & Utilities

## Description (plain text)

```
A one-time handwritten note for your wrist.

At the gym and need to remember your locker number? Open Scribbly Note, scribble "117" with your finger, press SELECT. Done. When you're back, open the app and your locker number is right there.

Also handy for parking spots, door codes, table numbers or a short shopping list.

- Opens straight to your last note
- Write with your finger on the touchscreen
- UP undoes the last stroke, SELECT finishes the note
- Press SELECT to start a new note; the old one is replaced
- Touch is ignored while viewing, so you can't scribble over your note by accident
- Saved after every stroke
- Works fully offline: no phone, account or internet needed

Requires a touchscreen (Pebble Time 2).

WARNING: All vibe-coded. I didn't do much. :')
```

## Release notes (1.0.0)

```
First release.
```

## Source code URL

https://github.com/MrDenexi/pebble-scribbly-note

## Screenshots (emery, 200×228)

Upload in this order:

1. `docs/screenshots/note.png`: a saved note (locker 117)
2. `docs/screenshots/writing.png`: writing mode
3. `docs/screenshots/new-note.png`: empty note with button hints
4. `docs/screenshots/menu.png`: the app in the watch menu

## Icons and banner (optional)

- Small icon 80×80, large icon 144×144: not made yet. Without them the store generates icons from the app name.
- Banner 720×320: not made yet.

## Before submitting

- [ ] `author` in `package.json` is still the template default (`MakeAwesomeHappen`). Set it to the name you want shown.
- [ ] `version` in `package.json` is `1.0.0`. Each later release needs a higher version.
- [ ] Upload `build/pebble-scribbly-note.pbw` from a fresh `pebble build`.
