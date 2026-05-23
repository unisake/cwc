# cwc

[日本語版はこちら](./りぃどみぃ.md)

**A Wayland compositor where all device interactions are controlled through a unified CLI.**

> No built-in UI. No forced layout. Just a protocol between your hardware and your tools.

---

## Why cwc?

Most compositors decide how your windows behave.

cwc does not. It exposes device management as simple CLI commands — and lets you, your scripts, and your tools decide everything else.

This follows the UNIX philosophy: do one thing well, compose the rest.

---

## How it works

cwc exposes three composable commands:

### `wm` — Window placement

```sh
# Launch an app (not yet visible)
firefox &

# Check running windows
wm
# /sys/fs/cgroup/user.slice/firefox.service/ <ID_A>

# Place it on screen declaratively
wm [0,0,1920,1080] <ID_A>
```

### `km` — Keyboard bindings

```sh
# Bind a key combination to a process
km [super,c] copy

# Longest-match wins — no conflicts
km [super] launcher
km [super,c] copy
```

### `pm` — Pointer bindings

```sh
# Bind right-click to a context menu process
pm [right] <context_menu_ID>

# Bind pointer movement to a cursor process
pm [pointer] <cursor_ID>
```

---

## Philosophy

- **Minimal core** — cwc manages devices. Nothing more.
- **No built-in UI** — tabs, launchers, taskbars: implement them as apps using cwc's API.
- **Declarative** — no auto-layout, no magic. Every change is explicit.
- **Composable** — pipe it, script it, drive it from JSON or any language.

---

## Architecture

```
┌─────────────────────────────────┐
│         CLI client              │  ← you / your scripts
│     (wm / km / pm / dev)        │
└────────────┬────────────────────┘
             │ UNIX socket
┌────────────▼────────────────────┐
│         core server             │  ← state manager
├──────────┬──────────┬───────────┤
│ wm logic │ km logic │ pm logic  │
└──────────┴──────────┴───────────┘
             │ DRM/KMS · libinput · EGL
┌────────────▼────────────────────┐
│           kernel                │
└─────────────────────────────────┘
```

---

## Status

> **Early development. Not yet usable as a daily driver.**

- [x] Core architecture design
- [ ] wm — basic window placement
- [ ] km — keyboard binding
- [ ] pm — pointer binding
- [ ] dev — device management

---

## Dependencies

- `wayland` + `wayland-protocols`
- `wlroots`
- `libdrm` · `libinput` · `xkbcommon`
- `EGL` / `Mesa`
- `systemd` / `logind`
- `meson` (build)

---

## License

MIT