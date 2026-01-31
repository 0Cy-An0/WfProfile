A Warframe Tool to track your Mastery progress. auto updates via the public export. Allows exporting your statistics to raw json or somewhat ordered csv
<img width="1161" height="617" alt="grafik" src="https://github.com/user-attachments/assets/2e17471c-4347-4052-b755-f84951bac3b4" />

<img width="1271" height="612" alt="grafik" src="https://github.com/user-attachments/assets/d42fc8f5-36dc-4427-9a38-b534931facd2" />

<img width="1159" height="618" alt="grafik" src="https://github.com/user-attachments/assets/e45f25a3-30c6-4349-ad76-a2fd395a3f17" />

With version 2 comes new options

<img width="477" height="348" alt="grafik" src="https://github.com/user-attachments/assets/e7fe1210-6a15-4ca8-bcd1-869fd6d501df" />

Also a fail save check. any request to the warframe api are throttled to 1 every 30seconds so that you wont get rate limited if you start spamming buttons. Keep in mind that i have made it intentionally so that the program starts not responding if you do try to. you do not want the api blocking your ip because you spammed them (things you wont be able todo if you are blocked include: logging into the game)
I also thought i should probably give a quick explaintion of the buttons. Update game data fetches the public export for the newest items in the game. sync inventory will send the request to the api for the current playerId you have put in (as if you were doing /profile in game) and save it in the data folder to read it. the Auto sync option uses the timer below to auto press this button. you can also tick "Sync on Mission finish" which will read warframe's EE.log for a log line that says you have finished a mission. The relic overlay similary reads ee.log for the prime reward popup. and will then display the currently loaded profiles information about the relevant items for each part you are seeing. Expor allows a curated/formatted .csv export and a raw .json export to get the profile more easily. Hope this explain a bit if you had trouble figuring out the buttons
