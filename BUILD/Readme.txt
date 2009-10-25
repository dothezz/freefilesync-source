FreeFileSync v3.1
-----------------

---------
| Usage |
---------

1. Choose left and right directories and "Compare" them.
2. Select synchronization settings and press "Synchronize..." to begin synchronization.


----------------
| Key Features |
----------------

1. Compare files (bytewise or by date) and synchronize them.
2. No limitations: An arbitrary number of files can be synchronized.
3. Unicode support.
4. Network support.
5. Synchronization database for automated setting of sync-directions and conflict detection
6. Support for multiple folder pairs with distinct configuration
7. Full support for Windows/Linux Symbolic Links and Windows Junction Points.
8. Lean & easily accessible UI: Highly optimized for speed and huge sets of data.
9. Algorithms coded in C++ completely.
10. Subfolders are also synchronized, including empty folders.
12. Progress indicators are updated only every 100ms for optimal performance!
12. Create Batch Jobs for automated synchronization with or without GUI.
13. Focus on usability:
	- Only necessary functionality on UI: no overloaded menus or icon jungle.
	- Select folders via drag & drop.
	- Last configuration and screen settings are saved automatically.
	- Maintain and load different configurations by drag&drop, load-button or during startup.
	- Double-click to show file in explorer. (Windows only)
	- Copy & paste support to export file-lists.
	- Delete superfluous/temporary files directly on main grid.
	- Right-click context menu.
	- Status information and error reporting
	- Sort file-lists by name, size or date.
	- Display statistical data: total filesizes, amount of bytes that will be transfered with the current settings.
14. Support for filesizes larger than 4 GB.
15. Option to move files to Recycle Bin instead of deleting/overwriting them.
16. Automatically ignore directories "\RECYCLER" and "\System Volume Information" with default Filter. (Windows only)
17. Localized versions for many languages are available.
18. Delete before copy: Avoid disc space shortages with large sync-operations.
19. Based on wxWidgets framework => Portable to many operating systems.
20. Filter functionality to include/exclude files from synchronization (without re-compare!).
21. Include/exclude specific files from synchronization manually.
22. Create sync jobs via GUI to synchronize automatically (can be scheduled or executed directly).
23. Handle daylight saving time changes on FAT/FAT32 volumes correctly.
24. Portable version (.zip) available.
25. No Windows registry entries for portable version.
26. Support for \\?\ path prefix for unrestricted path length. (Windows only)
27. Check for updates from within FreeFileSync automatically.
28. Copy locked files using Windows Volume Shadow Copy. (Windows only)
29. Load file icons asynchronously for maximum display performance.
30. Create regular backups with macros %time%, %date% within directory names
31. Copy file and folder create/access/modification times when synchronizing



-------------------
| Advanced topics |
-------------------

1.) Synchronize in Batch Mode and send error notification via email:

- Create a FreeFileSync batch file using "silent mode".
- Set error handling to "Exit with Returncode < 0" or "ignore errors" to avoid having a popup stop the program flow.
  In case errors occur FreeFileSync will abort with a returncode < 0 which can be checked via the ERRORLEVEL command.
- Create a *.cmd or *.bat file and specify the location of FreeFileSync.exe and pass the name of the FreeFileSync batch file as first argument; e.g.:

	C:\Program Files\FreeFileSync\FreeFileSync.exe C:\SyncJob.ffs_batch
	IF NOT ERRORLEVEL 0 echo An error occurred! && pause

- Instead of displaying "An error occurred!" you can specify any other command like sending an email notification (using a third party tool).


------------------------------------------------------------------------------------
2.)	Schedule Batch Job in Windows Task Planner

- Create a FreeFileSync batch file. (E.g. C:\SyncJob.ffs_batch)
- Create a new task in Windows Task planner for FreeFileSync.exe.
- Modify the task and adapt the execute-command specifying the path to the batch file; e.g.:
		
		C:\Program Files\FreeFileSync\FreeFileSync.exe C:\SyncJob.ffs_batch

		
------------------------------------------------------------------------------------		
3.) Drag & drop support

FreeFileSync has a big focus on usability. Therefore drag & drop is supported in various situations:

You can: - drag & drop any directory onto the main window to set the directory for comparison
         - drag & drop any file onto the main window to set the directory for comparison
         - drag & drop *.ffs_gui files onto the main window to load the configuration contained
         - drag & drop *.ffs_batch files onto the main window to display and edit the batch configuration
         - drag & drop *.ffs_batch files onto the batch dialog to display and edit the batch configuration


------------------------------------------------------------------------------------
4.) Exclude all subfolders from synchronization

If you want to synchronize all files from the base synchronization directories only, simply set up a filter like this:

	Include: *
	Exclude: *\
	
This will exclude all objects within the two directories that end with a "\" character, which is interpreted as the end of a directory name,


------------------------------------------------------------------------------------
5.) Synchronize with FTP

FreeFileSync does not support FTP directly. But the FTP functionality can be easily activated by mapping the FTP webspace to a drive letter:

Example: Use the free utility NetDrive (http://www.netdrive.net/)
- Add a "New Site" and specify site name, site URL, drive letter, account and password.
- Use the newly created drive as if it were a regular hard disk.
- Note: Most FTP drives set a file's timestamp to current time when synchronizing. As a workaround you can try a "compare by filesize", see below.


------------------------------------------------------------------------------------
6.) Start external application on double-click

FreeFileSync's default is to show files in the operating system's standard file browser on each double-click e.g. by invoking "explorer /select, %name" on Windows.
If some other application shall be started instead, just navigate to "Menu -> Advanced -> Global settings: External Applications" and replace the command string:

Examples:	- Start associated application:		cmd /c start "" "%name" 				
			- Start visual difference tool:		C:\Program Files\WinMerge\WinMergeU.exe "%name" "%nameCo"
(Don't forget to use quotation marks if file names contain spaces!)


------------------------------------------------------------------------------------
7. Synchronize USB sticks with variable drive letter

USB sticks often have different volume names assigned to them when plugged into two distinct computers. In order to handle this flexibility FreeFileSync is able to 
process directory names relative to the current working directory. Thus the following workflow is possible:
	
	- Replace the absolute USB directory name (variable) in your configuration by a relative one: E.g. "E:\SyncDir" -> "\SyncDir"
	- Save and copy synchronization settings to the USB stick: "E:\settings.ffs_gui"
	- Start FreeFileSync by double-clicking on "E:\settings.ffs_gui"

=> Working directory automatically is set to "E:\" by the operating system so that "\SyncDir" is interpreted as "E:\SyncDir". Now start synchronization as usual.


------------------------------------------------------------------------------------
8. Start RealtimeSync via commandline

RealtimeSync can be used to load a configuration file and start processing immediately without additional user intervention. Just pass the name of a configuration file as 
first commandline argument.

Example: C:\Program Files\FreeFileSync\RealtimeSync.exe MyConfig.ffs_real


------------------------------------------------------------------------------------
9. Compare by filesize

Sometimes you might want to compare both sides by filesize only, ignoring the last modification timestamp. Here is how you can do this: Open your *.ffs_gui configuration file
and change the XML node <FileTimeTolerance> to some sufficiently large value. Now changed files will be detected as a conflict (same date, different filesize) and a default
synchronization direction for conflics can be used.


------------------------------------------------------------------------------------
10. Create regular backups with time-stamped directory names

You can use macros %time%, %date% within the directory names you want to synchronize: Assuming you have a directory "C:\Source" which you want to backup each day into a time-stamped target
directory like "C:\Target_2009-10-08", all that needs to be done is setting up base directories like this:

Source folder: "C:\Source"
Target folder: "C:\Target_%date%"

Latter will be interactively replaced with the current date. In order to further automate this process, you can create a *.ffs_batch file with this configuration and choose
"ignore errors" to avoid the warning that target directory is not (yet) existing.



---------
| Links |
---------

FreeFileSync on SourceForge: 
http://sourceforge.net/projects/freefilesync/


------------
| Contact  |
------------

For feedback, suggestions or bug-reports you can write an email to:
zhnmju123 [at] gmx [dot] de

or report directly to:
http://sourceforge.net/projects/freefilesync/


Have fun!
-ZenJu
