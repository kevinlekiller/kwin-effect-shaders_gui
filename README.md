# KWin Effect Shaders GUI

Configuration GUI for https://github.com/kevinlekiller/kwin-effect-shaders

## Index
- [Installing](#installing)
- [Uninstalling](#uninstalling)
- [Keyboard Shortcut](#keyboard-shortcut)
- [Modifying The Shader Settings](#modifying-the-shader-settings)
- [Blacklisting Applications](#blacklisting-applications)
- [Whitelisting Applications](#whitelisting-applications)
- [Finding Application Names](#finding-application-names)
- [Enabling On Login](#enabling-on-login)

## Installing
    git clone https://github.com/kevinlekiller/kwin-effect-shaders_gui
    cd kwin-effect-shader_gui
    # Use the install.sh script:
    ./install.sh

## Uninstalling
    ./install.sh UNINSTALL

## Keyboard Shortcut
You can add a keyboard shortcut to start the gui:

`System Settings -> Workspace -> Shortcuts -> Custom Shortcuts`

`Edit -> New -> Global Shortcut -> Command/URL`

Double click on "New Action", change the name to something like "Shaders Config UI".

Go to the `Trigger` tab, add a keybind, for example `CTRL+META+X`

Go to the `Action` tab, add `kwin-effect-shaders_gui` to the command, press `Apply`.

![Modifying keyboard shortcuts](https://github.com/kevinlekiller/kwin-effect-shaders_gui/raw/main/images/keyboard_shortcut.png)

## Modifying The Shader Settings
Open the configuration UI (see [Keyboard Shortcut](#keyboard-shortcut)), go to the `Shaders` tab.

Click on the shader you want to enable, click `Save`.

You can also enable `Auto Save` in the `Settings` tab, which will automatically save the settings.

![Shaders tab](https://github.com/kevinlekiller/kwin-effect-shaders_gui/raw/main/images/shader_configuration.png)

## Blacklisting Applications
In the configuration UI, in the `Settings` tab, you can add application(s), if more than 1, seperate them with a comma.

For example: `plasmashell,Firefox`

This will block the Plasma desktop and Firefox from being processed.

The list is not case sensitive.

## Whitelisting Applications
In the configuration UI, in the `Settings` tab, you can add application(s), if more than 1, seperate them with a comma.

For example: `kate,kcalc`

Only Kcalc and Kate will be processed.

The list is not case sensitive.

This is useful if you use the `Auto Enable` option.

You can use the `Blacklist` at the same time as the `Whitelist`.

## Finding Application Names
To find an application name to use in the blacklist or whitelist, follow these steps:

`System Settings -> Window Management -> Window Rules -> Add New...`

If you're in a game, set it to windowed mode.

Click `Detect Window Properties`, click the application.

The application name is first string in the `Whole window class` (delimited by the space character).

For example, with kate, the "Whole window class" is `kate org.kde.kate`, so the name to put in the `Blacklist` or `Whitelist` would be kate.

Another example, lutris, the whole window class is `lutris` so the name would be lutris.

![Find the application name](https://github.com/kevinlekiller/kwin-effect-shaders_gui/raw/main/images/find_application_name.png)

## Enabling On Login
In the configuration UI, in the `Settings` tab, you can set the `Auto Enable` option.

This will process all applications on login.

You can exclude applications by using the `Blacklist` and/or `Whitelist`.
