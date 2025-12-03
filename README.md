# 🕰️ WatchWinder 3000

**The Over-Engineered Solution to Keeping Your Automatic Watch Ticking!**

Welcome to **WatchWinder**, an Arduino-powered contraption that ensures your automatic watch never runs out of juice while you're sleeping, coding, or just being lazy. Controlled by an infrared remote and featuring a dazzling LED matrix display, this project turns a mundane task into a light show!

## ✨ Features

- **remote Control Magic:** drive the motor from across the room using a standard IR remote.
- **Matrix Display:** The built-in LED matrix on the UNO R4 WiFi talks to you! See status updates like "RDY", "LEFT", and real-time position tracking.
- **Siren Mode:** When continuous winding is active, the RGB LED blinks Red and Blue like a tiny police car chasing a time thief.
- **Precision Winding:** Pre-programmed modes for specific rotation angles and speeds.
- **Safety First:** Includes a 10-minute auto-shutoff so you don't accidentally wind your watch into the 4th dimension.

## 🛠️ Hardware Stack

- **Brain:** Arduino UNO R4 WiFi (Renesas RA)
- **Muscle:** Stepper Motor (28BYJ-48 + ULN2003 Driver)
- **Eyes:** IR Receiver Module
- **Bling:** RGB LED (Common Cathode)
- **The Box:** A 3D printed case (I am using https://github.com/mwood77/osww as my case.)

## 🎮 How to Pilot

Grab your IR Remote and let's wind!

### 🔄 Continuous Mode (The "Set It and Forget It")
- **⬅️ Left Arrow:** Spin Counter-Clockwise endlessly (well, for 10 mins).
- **➡️ Right Arrow:** Spin Clockwise endlessly.
- **🆗 OK Button:** **STOP!** (Halts the continuous madness).

*Note: While in this mode, the RGB LED goes into "Siren Mode" (Red/Blue flash).*

### 🎯 Precision Moves (The "Tactical Winding")
Use the number pad for specific maneuvers:

| Button | Direction | Distance | Speed |
| :---: | :---: | :---: | :---: |
| **1** | ↺ CCW | 1/8 Turn | 🐢 Slow |
| **2** | ↺ CCW | 1/4 Turn | 🚶 Normal |
| **3** | ↺ CCW | 1/2 Turn | 🐇 Fast |
| **4** | ↻ CW | 1/8 Turn | 🐢 Slow |
| **5** | ↻ CW | 1/4 Turn | 🚶 Normal |
| **6** | ↻ CW | 1/2 Turn | 🐇 Fast |

## 🌈 Status Lights Decoder

- **🔴 Red (Solid):** Busy / Starting up. Don't touch!
- **🟢 Green (Solid):** Ready. Awaiting orders, Captain.
- **🚨 Red/Blue (Blinking):** Winding in progress! Stand back!

## 🚀 Getting Started

1.  **Clone it:** Download this repo.
2.  **Open it:** Use VS Code with the PlatformIO extension.
3.  **Build it:** Hit that checkmark button.
4.  **Flash it:** Upload code to your Arduino UNO R4 WiFi.
5.  **Wind it:** Strap your watch in and press play.

---

*Built with ❤️, stepper motors, and too much caffeine.*
