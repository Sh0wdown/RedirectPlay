# RedirectPlay

## Overview

This library allows to establish a DirectPlay4 session through Steam using the public APIs of both.
It extends DirectPlay functionality by adding a new "Service Provider" as an option for the user if Steam is running.
By choosing this new "Steamworks Connection" Service Provider networking will be done through the Steamworks API.
Sessions can be created public or for friends only.

## Installation & Usage
- Copy the dplay.dll and steam_api.dll into the game's folder. (Only the dplay.dll is project code, the steam_api.dll could also be taken from the Steamworks SDK.)

That's it! Now if Steam is running and everything worked out the game should be showing "Steamworks Connection" in the service provider's list (next to TCP/IP etc.).
Choose that entry to join or host a server. When hosting a message box should pop up for server settings (Name, password etc.).

Have fun!

## Issues
A lot can still be added and improved, but the current version is already working.
- Steam friends' "Invites" and "Join" do not work. However, Friends' servers are tagged and should be at the top in the server list (unless the game resorts the entries).
- Host migration is not implemented. You'll have to create a new session if the host quits.
- Steam & [DxWnd](https://github.com/DxWnd) seem to not work together.
- Only the DirectPlay wide char interface is implemented. Games that use the Ansi interface might not work.

## Technical Details

The RedirectPlay library functions as a layer between the original DirectPlay library and the application, it is not a replacement. More options are added to the available service providers and only if a new options is chosen
RedirectPlay will provide functionality. Otherwise all calls will be forwarded to the DirectPlay library.
The new Steam ServiceProvider implements the DirectPlay4 interface through the [Steamworks API](https://partner.steamgames.com/doc/sdk).

Usually DirectPlay is used through [Microsoft's Component Object Model (COM)](https://learn.microsoft.com/en-us/windows/win32/com/component-object-model--com--portal).
Since DirectPlay is a (deprecated) part of Windows, I don't want to replace the file, which makes it difficult to add functionality without code injection.
However, if an application not only uses DirectPlay through COM, but also through its exports, the library will be loaded directly.
In this case it is enough to just put a custom library file in the application's directory, and hook the process' COM call to make sure this library is used for specific CLSIDs.

## Compiling the code
To compile the code you need public headers from the following SDKs:
[Steamworks SDK](https://partner.steamgames.com/downloads/list) - Copy the headers from \public\steam\ to RedirectPlay\External\Steam\.
[DirectX8 SDK](https://archive.org/details/dx8sdk) - Copy dplay.h and dplobby.h from \include\ to RedirectPlay\External\DirectX\.
