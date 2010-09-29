AnyTitleDeleter 1.1
----------------------
original code	by tona
database.txt	by Red Squirrel
database code	by MrClick
.app fix and
tmd check	giantpune
cleanup, etc	bushing

Features:
This program will delete (almost) any title on your system--ticket, title, 
and contents. It contains brick protection, so you shouldn't have to worry
about bricking your system by deleting the wrong thing.
When possible the title's banner.bin and 00000000.app (stored on the NAND)
or the external database (database.txt in the SD cards root) will be
consulted to determine the name of a title.
Title names read from the banner.bin can be saved to database.txt from
within the program to complete the database. The internal names are marked
by asterisks in the list. So when in (SD > NAND) mode all marked titles have
a proper internal name but no entry in the database.
Titles can be added to the database when an entry already exists creating a
duplicate for correction purposes.

FROM giantpune...
I changed the code to not only look at the 00000000.app, but instead to look 
at the first .app for a given title.  This way it can get the correct .app for 
non-homebrew programs.  Also, if i title is type 0x10000, it checks the data
content folders and if it only finds a title.tmd, it will display tmd! before
the name.

For a full description, see
http://wiibrew.org/wiki/Homebrew_apps/AnyTitle_Deleter

Disclaimer:
THIS SOFTWARE COMES WITH NO WARRANTY!
Although there is no reason at hand why this could brick your system, 
in the event that a bug or a future Wii update has allowed you to damage
your system in any way, the author of this software cannot be held 
responsible for you loss. That said, if you notice any errors, please
report them to the author in order to help prevent bricks.

Source code:
Included with a simple, lienient license/policy.
It's not as pretty as it could be, but I need to make a release.
I hope it will serve some purpose to others.

Database:
The database file has to be copied to the root of the SD card.
