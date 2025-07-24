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

= = = = = = = = = = = = = = =

< Why did I release this source code? >

When I first started making GSC Injector last year, I had no idea how to use IDAPro, so Sku taught me a lot of the basics and how to operate it.

But as I worked with Sku to create GSC Injectors for many game titles, such as MWII and Vanguard, I got used to using IDAPro.

This MW19 Beta Injector is one of them, and I found it all by myself, dumping MW19, without any help from Sku.

If we were to actually make GSC Injector, everyone would have to relive the many hours of trial and error that Sku and I spent, which would be very difficult.

I released this source code thinking of ambitious developers who want to make new GSC Injectors in the future.

If you are a developer viewing this repository now and have knowledge of C++ and IOAPro, you can simply change a few addresses in this source and correct the Load_ScriptFile processing to the way it was done in IDAPro, and you will be able to inject GSC in any version of MW19.

However, I can't say anything about whether gsc-tool will need to be modified...

= = = = = = = = = = = = = = =

< To everyone who modifies this source to create a GSC Injector >

Obviously, this is a freely released source, so if you use this source to complete a GSC Injector, it is up to you what to do with it.

You can release it for free to the majority of people, gatekeep it, or sell it for a fee - it's all up to you.

However, if you do release it, please consider the compensation you will receive for that act.

The hardships you went through to complete the GSC Injector, the time it took to develop it, etc.

Other people who will use your GSC Injector will not know that you took the trouble to analyze it with IDAPro, and while you were working hard to create your GSC Injector, others were just playing around lazily without developing it.

Taking these facts and the benefits to you into account, please take your time to decide how to release the completed GSC Injector.

You should decide the value of the GSC Injector you created.

= = = = = = = = = = = = = = =

< Why do you sell it for a fee? >

I have already released free GSC Injector for several CoDs, including this source, Vanguard Beta, MWII CP, etc.

The same goes for GSC Mod Menu.

MW19 1.20, 1.67, PS4, BOCW Defcon, Vanguard Beta, MWII CP, BO6 Alpha, etc.

I have released the source and compiled Mod Menu for GSC Mod Menu for free for many CoDs.

However, as mentioned above, many people will not want to know the fact that I have spent a lot of time and effort creating this Mod Menu, Injector, and Loader, and they will not be interested.

Also, releasing it for free does not generate any merit for my development time and effort.

For that reason, I have made some of the GSC Injectors for retail and specific builds private, but I will give them away for a fee if there is strong demand.

I think there are many people who want to get and use my GSC Injector and GSC Mod Menu for free.

There's no need to pay, that's why I'm releasing the source code for these GSC Injectors and GSC Mod Menus.

If you want GSC Injector for free, please get IDAPro or Ghidra and modify this source.

You can choose to modify this Beta GSC Injector for free, and I support whatever choice you make.
