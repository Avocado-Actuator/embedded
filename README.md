# Avocado Microcontroller Code

This project contains the code that runs on the Avocado microcontroller.

## Setup CCS

Instructions for using CCS with this repo:

1. Make a new empty CCS project
    - Target: Tiva C Series - Tiva TM4C1294NCPDT
    - In project templates select `Empty Project`
2. Clone this repo (or move the folder if already cloned) into this new project folder.
3. Now move all files from this repo into the project folder (including the hidden `.git` and `.gitignore`). Once these files have been copied over you can delete the cloned folder.
4. The project folder is now your git repo. Use git commands there like normal (the `.gitignore` file will ignore CCS configuration files). If you run `git status` you should not have any unstaged changes (if you see an unstaged `targetConfigs` folder feel free to delete it)
5. Now we need to set a few CCS project properties:
    - Under `Build -> ARM Compiler -> Include Options` in the `Add dir to #include search path` pane add your TivaWara C Series folder (the folder that contains your examples, driverlib, etc.). For me this folder is `~/ti/SW-TM4C-2.1.4.178` (*Note:* the folder name may be different in Windows, an example path is `C:\ti\TivaWare_C_Series-2.1.4.178`)
      ![Include Options](https://raw.githubusercontent.com/Avocado-Actuator/embedded/assets/images/include_options.png)
    - Next under `Build -> ARM Linker -> File Search Path` in the `Include library file or command file as input` pane add the full path to your `driverlib.lib` file. For me this path is `~/ti/SW-TM4C-2.1.4.178/driverlib/ccs/Debug/driverlib.lib`, on Windows an example path might be `C:\ti\TivaWare_C_Series-2.1.4.178\driverlib\ccs\Debug\driverlib.lib`.
      ![File Search Path](https://raw.githubusercontent.com/Avocado-Actuator/embedded/assets/images/file_search_path.png)
    - Penultimately, under `Build -> ARM Compiler -> Predefined Symbols` in the `Pre-define NAME` pane add `TARGET_IS_TM4C129_RA0`.
      ![Predefined Symbols](https://raw.githubusercontent.com/Avocado-Actuator/embedded/assets/images/predefined_symbols.png)
    - Finally, under `Build -> ARM Compiler -> Basic Options` set the `Set C system stack size` option to `65536`.
      ![Stack Size](https://raw.githubusercontent.com/Avocado-Actuator/embedded/assets/images/stack_size.png)
6. Go ahead and build! If something is still wrong let us know and we'll try and help :)

