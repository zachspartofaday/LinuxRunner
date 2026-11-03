# Hardware keyboard and pointer forwarding

Hardware Shift now uses UIKit's actual layout-aware printable character while
Control/Option/Command shortcuts retain printable X11 keysyms. Key-up reuses the
key-down symbol. A modifier's own key-down updates its state immediately.

UIKit remains the source of Magic Keyboard primary trackpad clicks when
GameController exposes movement but no primary-button callback. Policies avoid
duplicate clicks when GameController supplies that button. Clicking a Linux
window acquires native keyboard focus when a hardware keyboard is attached.
Pointer-button history is per viewer and release resets it.

Host policy checks cover modifiers/layouts, source transitions and pointer
button ownership. These checks do not synthesize real Magic Keyboard hardware.
Earlier manual integration use reported working clicks, typing and Shift; a
standalone physical-device qualification is not claimed by this extraction.
