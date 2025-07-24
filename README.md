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

= = = = = = = = = = = = = = =

< Summary >

I had a big conflict with a Warzone AIO creator last year.

When he was talking about whether to release GSC Injector or not, I didn't want to wait for him to release it.

So I argued with him, and I started learning IDAPro from Sku to port Load_ScriptFile.

Because of that opportunity, I can now reverse engineer and create software on my own to a certain extent.

Without that effort, the free GSC Injector source code would not have been released here now...

(By the way, after various things happened and I reconciled with him.)

Not only Injector, but my GSC Mod Menu is also like that.

Those who know Maza and others will know that when I first joined Donetsk, the first GSC I released had much worse source code than it does now.

Even from that time, while many general users of Donetsk were just playing, I continued to develop and research GSC.

Thanks to that effort, mjkzy recognized it and shared iw8_mod with me. (Not iw8-mod)

This allowed me to develop at a much faster pace and make non-standard mods.

I continued to develop and improve it over and over again for months.

The result is the current Project HiNAtyu GSC Mod Menu.

In the end, there is only one thing I want to say.

That is, "You cannot complete a work without effort."

I think it is difficult to get something good for free without effort.

If you can't get what you want for free, before discussing free or paid, first make an effort to modify the published source code.

Only those who have made that effort should be able to discuss free or paid.

Those who are not on the same developer's stage should learn coding and reverse engineering before arguing.

You should first find out who spent a lot of time and effort analyzing the AIO you are using for free...

= = = = = = = = = = = = = = =

< Finally, about a certain Warzone AIO >

The GSC Injection process that is executed by pressing the "Load script" button is not the Load_ScriptFile used in this source.

It is a different process called Asset Pool, which I will not cover in my source, so please look into it yourself.

Also, Asset Pool can change multiple GSCs.

However, I don't think you should request that the creator of the AIO be able to change multiple GSCs with Asset Pool, and I hope you don't ask for anything either.

He threw away the above benefits and compensation and released the AIO for free.

The AIO should be fully paid, but since it is released for free, the AIO creator is not paid anything.

You should not complain about the process that he spent months creating, even though it is a free release.

If there is anyone who wants to rewrite multiple GSCs with Asset Pool GSC Injector, first try learning Asset Pool without complaining.

Then try making your own Asset Pool GSC Injector and distribute it to everyone in the community for free.

I think that's the attitude that someone seeking improvements to an Injector that has been released for free should have.

Incidentally, I learned Asset Pool in order to make PS4 GSC Injectors for MWII and MWIII.

You should also try your best to develop something; after using IDAPro for about a year, you'll be able to do a certain amount of reverse engineering.

You can choose to modify this Beta GSC Injector for free, and I support whatever choice you make.
