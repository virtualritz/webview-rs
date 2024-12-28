<!--lint disable no-literal-urls-->
<div align="center">
  <h1>webview-rs</h1>
</div>
<br/>
<div align="center">
  <strong>
      <a href="https://github.com/chromiumembedded/cef">Chromium Embedded Framework (CEF)</a>
       bindings for rust.</strong>
</div>
<div align="center">
  <img src="https://img.shields.io/github/actions/workflow/status/mycrl/webview-rs/release.yml?branch=main"/>
  <img src="https://img.shields.io/github/license/mycrl/webview-rs"/>
  <img src="https://img.shields.io/github/issues/mycrl/webview-rs"/>
  <img src="https://img.shields.io/github/stars/mycrl/webview-rs"/>
</div>
<div align="center">
  <sup>
    current version: 
    <a href="https://cef-builds.spotifycdn.com/index.html#windows64:116.0.22%2Bg480de66%2Bchromium-116.0.5845.188">116.0.22+g480de66+chromium-116.0.5845.188</a>
  </sup>
  </br>
  <sup>platform supported: Windows / Linux(x11)</sup>
</div>

--- 

Inspired by an internal company project, but not a complete copy. My main job at my previous company was to integrate CEF into the Rust client, but the internal project made a lot of specific changes to accommodate business needs, which wasn't to my liking, so I decided to create a more generic version myself.

Would this be an infringement of intellectual property rights? I don't have a definite answer, after all, I used my experience from working at the company, but it's not like I copied the code exactly and opened it up. To avoid this problem, I will not be providing active support for this project and this is just my own version.

## Usage

Cargo has a very poor experience when relying on dynamic libraries, this cannot be fixed at the compiled script level, at least not by publishing directly to crates.io, if you want to run an example, see this issue: https://github.com/mycrl/webview-rs/issues/3

## License
[MIT](./LICENSE) Copyright (c) 2022 Mr.Panda.
