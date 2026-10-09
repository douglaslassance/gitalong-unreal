# gitalong-unreal

An [Unreal Engine](https://www.unrealengine.com) revision control plugin that integrates [Gitalong](https://github.com/douglaslassance/gitalong) to prevent conflicts when working with a team.

It is built on top of the [UEGitPlugin](https://github.com/SRombauts/UEGitPlugin) written by [Sébastien Rombauts](https://github.com/SRombauts) and is distributed under the same [MIT license](LICENSE.txt).

## Pre-requisites

- [Unreal Engine 5.8](https://www.unrealengine.com/download)
- [Git 2.35](https://git-scm.com/) or later
- [Gitalong 1.1](https://github.com/douglaslassance/gitalong) or later

## Installation

The recommended way to install this plugin is to submodule this repository inside the `Plugins` folder of a code project.

```shell
git submodule add https://github.com/douglaslassance/gitalong-unreal Plugins/Gitalong
```

The plugin replaces the Git plugin that ships with the engine for that project.

> [!IMPORTANT]
> We currently don't provide a built version of the plugin therefore non-code projects will not be able to use the plugin without extra work on your part.

## Repository setup

Every clone of the repository needs to be set up for Gitalong. Tracking uncommitted changes is what lets checkouts reach the rest of the team, so it is strongly recommended.

```shell
gitalong setup --track-uncommitted --update-hooks --tracked-extensions .uasset,.umap
```

Add `--modify-permissions` to keep files read-only until they are checked out. Refer to the [Gitalong documentation](https://github.com/douglaslassance/gitalong) for the available stores.

## Usage

Pick **Gitalong** as the provider in the editor revision control settings. The plugin finds `git` and `gitalong` in the usual install locations and on the `PATH`, and both paths can be overridden there.

- Checking out a file claims it with `gitalong claim`. The claim is refused when someone else is changing the file.
- Files claimed in another clone, committed but not pushed, or changed on another branch show as checked out by someone else.
- Files changed on the remote branch show as not current until synced.
- Reverting a file releases its claim, and pushing releases the files of the pushed commits.

You should be able to submit, revert, sync and push using the editor or any Git interface interchangeably.

Without `--track-uncommitted` the plugin behaves like the plain Git plugin, where every tracked file is editable, but still reports the changes of other clones.

## License

[MIT](LICENSE.txt)
