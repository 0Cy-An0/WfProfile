# Warframe Mastery Progress Tracker

A tool to track your **Warframe Mastery progress**.  
Updates via the official Warframe public export.

---

## Usage Notes

- **Safe to Use:**  
  The tool operates **entirely outside of Warframe**. It only reads files (like `EE.log`) and uses the **public Warframe API**, so it should be safe. If you want to be sure—**you can use it without running Warframe**.
  
- **Buttons Overview:**
  - **Update Game Data:** Fetches the newest item list from the Warframe export.
  - **Sync Inventory:** Retrieves your account data via the API for the current `playerId` and saves it locally.
  - **Auto Sync:** Enables automatic syncing at the set interval.
  - **Sync on Mission Finish:** Watches `EE.log` and triggers a sync when a mission ends.
  - **Relic Overlay:** Displays item mastery info related to relic parts when a prime reward popup is detected.
  - **Export:** Saves your data as either a formatted `.csv` or raw `.json`.

---

## Request Limits

To prevent you from getting API rate limited, the app enforces **a 30-second cooldown** between API requests.  
If you spam buttons too quickly, the program will intentionally pause and appear to “not respond” — this is by design, to protect your IP from being blocked.

(If the api blocks your ip, you will be unable to log into the game temporarly, as this requires communication with the api)

---

## Screenshots

![screenshot1](https://github.com/user-attachments/assets/2e17471c-4347-4052-b755-f84951bac3b4)
![screenshot2](https://github.com/user-attachments/assets/d42fc8f5-36dc-4427-9a38-b534931facd2)
![screenshot3](https://github.com/user-attachments/assets/e45f25a3-30c6-4349-ad76-a2fd395a3f17)
new in version 2:
![screenshot4](https://github.com/user-attachments/assets/e7fe1210-6a15-4ca8-bcd1-869fd6d501df)
