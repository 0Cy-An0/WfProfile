# Warframe Mastery Progress Tracker

A tool to track your **Warframe Mastery progress**.  
Updates via the official Warframe public export.

---
## Importanted Note

As of Update 43, the automatic profile ID detection method is no longer reliable, because Warframe no longer prints the profile ID in `EE.log`. The profile update that used to rely on `http://content.warframe.com/dynamic/getProfileViewingData.php?` also appears to have changed or been deprecated, so the (auto) sync does not work anymore.

It is neccesary to manually retrieve your account ID and profile data for now. To do that, log in to your Warframe account on `warframe.com`, open `https://www.warframe.com/api/user-data`, and look for your `user_id` value.

Then open the profile-viewing URL for your platform, replacing `ACCOUNTID` with that value and copy the text into `\WFProfil\data\Player\player_data.json` replacing any old data.
- PC: `https://api.warframe.com/cdn/getProfileViewingData.php?playerId=ACCOUNTID`
- PlayStation: `https://api-ps4.warframe.com/cdn/getProfileViewingData.php?playerId=ACCOUNTID`
- Xbox: `https://api-xb1.warframe.com/cdn/getProfileViewingData.php?playerId=ACCOUNTID`
- Switch: `https://api-swi.warframe.com/cdn/getProfileViewingData.php?playerId=ACCOUNTID`
- iOS: `https://api-mob.warframe.com/cdn/getProfileViewingData.php?playerId=ACCOUNTID`
- Android: `https://api-and.warframe.com/cdn/getProfileViewingData.php?playerId=ACCOUNTID`

This Tool should otherwise be uneffected and will still show you Progress as shown in the screenshots below.

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
