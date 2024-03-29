# KWin Effect Shaders GUI
Configuration GUI for https://github.com/kevinlekiller/kwin-effect-shaders
![GUI](https://github.com/kevinlekiller/kwin-effect-shaders_gui/raw/main/images/GUI.png)
## Installing
    git clone https://github.com/kevinlekiller/kwin-effect-shaders_gui
    cd kwin-effect-shaders_gui
    # Use the install.sh script:
    ./install.sh
## Uninstalling
    sudo rm -f /usr/local/bin/kwin-effect-shaders_gui
## Keyboard Shortcut
You can add a keyboard shortcut to start the gui:

`System Settings -> Workspace -> Shortcuts -> Custom Shortcuts`\
`Edit -> New -> Global Shortcut -> Command/URL`

Change the name from "New Action" to something like "Shaders Config UI".\
In the `Trigger` tab, add a keybind, for example `CTRL+META+X`\
In the `Action` tab, add `kwin-effect-shaders_gui` to the command, press `Apply`.

![Modifying keyboard shortcuts](https://github.com/kevinlekiller/kwin-effect-shaders_gui/raw/main/images/keyboard_shortcut.png)
## Modifying The Shader Settings
Open the configuration UI (see [Keyboard Shortcut](#keyboard-shortcut)), go to the `Shaders` tab.\
Click on the shader you want to enable, click `Save`.\
You can also enable `Auto Save` in the `Settings` tab, which will automatically save the settings.\
## Whitelisting Applications
In the configuration UI, in the `Whitelist` tab, you can add application(s), if more than 1, seperate them with a comma.\
For example: `kate,kcalc`\
Only Kcalc and Kate will be processed.\
The list is not case sensitive.\
This is useful if you use the `Auto Enable` option.\
The Whitelist is saved to the current profile.\
## Finding Application Names
To find an application name to use in the whitelist, follow these steps:

`System Settings -> Window Management -> Window Rules -> Add New...`

If you're in a game, set it to windowed mode.\
Click `Detect Window Properties`, click the application.\
The application name is first string in the `Whole window class` (delimited by the space character).\
For example, with kate, the "Whole window class" is `kate org.kde.kate`, so the name to put in `Whitelist` would be kate.\
Another example, lutris, the whole window class is `lutris` so the name would be lutris.

With Steam games, the name is `steam_app_APPID` (APPID is the App ID of the game, for example, with Half Life 2, it's 220 : https://steamdb.info/app/220/).

![Find the application name](https://github.com/kevinlekiller/kwin-effect-shaders_gui/raw/main/images/find_application_name.png)
## Profiles
You can add, remove, copy profiles in the Profiles tab.\
Rename a profile by double clicking it.
## Enabling On Login
In the configuration UI, in the `Settings` tab, you can set the `Auto Enable` option.\
This will process all applications on login.\
You can process specific applications using the `Whitelist`.
