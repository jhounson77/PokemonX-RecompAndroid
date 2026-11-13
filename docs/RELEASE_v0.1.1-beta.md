# V0.1.1 Beta

Android version name: `0.1.1-beta`; version code: **35**.

Fixes the automatic return to the primary battle screen when the Bag or party selection module closes while other auxiliary modules remain resident. The previous implementation waited for every auxiliary module to unload, which could leave the lower screen enlarged after using an item or switching Pokémon.

A newly loaded selection module still cancels the pending return, preventing a Bag-to-target transition from being treated as a completed action. Closing only a Pokémon summary viewer does not close the party selection. Reused menus loaded before battle are recognized when they close, even without a new load callback. Manual and dual-screen modes retain their existing behavior.

Regression checks exercise the production load/unload callbacks with resident helpers, all supported Bag/party roots, nested target selection, summary viewing and battle teardown. These synthetic checks do not establish that every actual game menu unloads its module; gameplay confirmation on the target phone remains required.

Install over the previous Beta with the same package and signing certificate; back up saves first.
