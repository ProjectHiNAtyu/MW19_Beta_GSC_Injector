[ About ]


This GSC Injector's source code is based on the GSC Injection process from Project Donetsk.

I would like to express my utmost respect and gratitude to Mr. Sku-111 for devising and implementing this injection process.

Thank you always, and I support you.


----------


[ Compiler ]


If you want to run custom GSC, you need to modify gsc-tool.

always, and I support you.


----------


[ A word ]


< Approach >

The source code for this GSC Injector was reworked in the first half of 2024 with Sku-111 from the GSC Injection process devised in Project Donetsk by SKu-111.

In Donetsk, the discord_game_sdk.dll, which is loaded by default when the game starts, was modified and the GSC Injector process was built into it, so GSC could be injected.

However, in the Retail version, the modified discord_game_sdk.dll is overwritten by the default dll before launching due to file integrity checks by Steam and BattleNet.

Also, the source code of the offline build dlls released by .r4v3n and h00dbyair is not public, so it is not possible to add GSC Injector functionality to the dll itself.

This project was created in anticipation of cases where it is not possible to modify discord_game_sdk.dll.

This project is designed to inject this GSC Injector dll into the running game Call of Duty Modern Warfare through an external dll Injector.

After injecting the dll, it attempts to hook the Load_ScripFile function that actually loads GSC in MW2019.

If the hook is successful, it reads the GSCBIN file located on the local drive and replaces it with the official GSC that was just about to be loaded in the game.

This is an overview of the processes performed in this project.
