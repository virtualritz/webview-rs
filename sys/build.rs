use anyhow::{anyhow, Result};
use bzip2::read::BzDecoder;
use reqwest::blocking::Client;
use std::{
    env,
    fs::File,
    io::copy,
    path::{Path, PathBuf},
    process::Command,
};
use tar::Archive;

fn download_file<P>(url: &str, output_path: P) -> Result<()>
where
    P: AsRef<Path>,
{
    let client = Client::new();

    // Send a GET request and download directly
    let mut response = client.get(url).send()?;

    // Create output file
    let mut output_file = File::create(&output_path)?;

    // Copy response directly to file
    copy(&mut response, &mut output_file)?;

    Ok(())
}

fn exec<P>(command: &str, work_dir: P) -> Result<String>
where
    P: AsRef<Path>,
{
    let output = Command::new(if cfg!(windows) { "powershell" } else { "bash" })
        .arg(if cfg!(windows) { "-command" } else { "-c" })
        .arg(if cfg!(windows) {
            format!("$ProgressPreference = 'SilentlyContinue';{}", command)
        } else {
            command.to_string()
        })
        .current_dir(work_dir)
        .output()?;

    if !output.status.success() {
        Err(anyhow!("{}", String::from_utf8(output.stderr)?))
    } else {
        Ok(String::from_utf8(output.stdout)?)
    }
}

fn main() -> Result<()> {
    let target = env::var("TARGET")?;
    let out_dir = PathBuf::from(env::var("OUT_DIR")?);
    let cef_path = out_dir.join("cef");

    println!("cargo:rerun-if-changed=./cxx");
    println!("cargo:rerun-if-changed=./build.rs");

    if !cef_path.exists() {
        #[cfg(target_os = "windows")]
        {
            exec(
                "Invoke-WebRequest -Uri https://github.com/mycrl/webview-rs/releases/download/distributions/cef-windows.zip -OutFile ./cef.zip",
                &out_dir,
            )?;
            exec("Expand-Archive -Path cef.zip -DestinationPath ./", &out_dir)?;
            exec("Remove-Item ./cef.zip", &out_dir)?;
        }

        #[cfg(target_os = "macos")]
        {
            exec(
                "wget https://github.com/mycrl/webview-rs/releases/download/distributions/cef-macos.zip -O ./cef.zip",
                &out_dir,
            )?;
            exec("tar -xf ./cef.zip -C ./", &out_dir)?;
            exec("rm -f ./cef.zip", &out_dir)?;
        }

        #[cfg(target_os = "linux")]
        {
            let archive_url = "https://cef-builds.spotifycdn.com/cef_binary_134.3.8%2Bgfe66d80%2Bchromium-134.0.6998.166_linux64_minimal.tar.bz2";
            let folder_name = "cef_binary_134.3.8+gfe66d80+chromium-134.0.6998.166_linux64_minimal";

            let archive_path = out_dir.join("cef.tar.bz2");
            download_file(archive_url, &archive_path)?;

            let decompressed = BzDecoder::new(File::open(archive_path)?);
            Archive::new(decompressed).unpack(&out_dir)?;

            std::fs::rename(out_dir.join(folder_name), &cef_path)?;
        }
    }

    #[cfg(not(target_os = "macos"))]
    if !cef_path.join("libcef_dll_wrapper").exists() {
        exec("cmake -DCMAKE_BUILD_TYPE=Release .", &cef_path)?;
        exec("cmake --build . --config Release", &cef_path)?;
    }

    bindgen::Builder::default()
        .default_enum_style(bindgen::EnumVariation::Rust {
            non_exhaustive: false,
        })
        .prepend_enum_name(false)
        .derive_eq(true)
        .size_t_is_usize(true)
        .header("./cxx/webview.h")
        .generate()?
        .write_to_file(&out_dir.join("bindings.rs"))?;

    let mut cfgs = cc::Build::new();
    let is_debug = env::var("DEBUG")
        .map(|label| label == "true")
        .unwrap_or(true);

    cfgs.cpp(true)
        .debug(is_debug)
        .static_crt(true)
        .target(&target)
        .warnings(false)
        .out_dir(&out_dir);

    if cfg!(target_os = "windows") {
        cfgs.flag("/std:c++20");
    } else {
        cfgs.flag("-std=c++20");
    }

    cfgs.file("./cxx/app.cpp")
        .file("./cxx/browser.cpp")
        .file("./cxx/control.cpp")
        .file("./cxx/render.cpp")
        .file("./cxx/display.cpp")
        .file("./cxx/webview.cpp")
        .file("./cxx/scheme_handler.cpp");

    cfgs.include(cef_path.clone());

    #[cfg(target_os = "windows")]
    cfgs.define("WIN32", Some("1"))
        .define("_WINDOWS", None)
        .define("__STDC_CONSTANT_MACROS", None)
        .define("__STDC_FORMAT_MACROS", None)
        .define("_WIN32", None)
        .define("UNICODE", None)
        .define("_UNICODE", None)
        .define("WINVER", Some("0x0A00"))
        .define("_WIN32_WINNT", Some("0x0A00"))
        .define("NTDDI_VERSION", Some("NTDDI_WIN10_FE"))
        .define("NOMINMAX", None)
        .define("WIN32_LEAN_AND_MEAN", None)
        .define("_HAS_EXCEPTIONS", Some("0"))
        .define("PSAPI_VERSION", Some("1"))
        .define("CEF_USE_SANDBOX", None)
        .define("CEF_USE_ATL", None)
        .define("_HAS_ITERATOR_DEBUGGING", Some("0"));

    #[cfg(target_os = "linux")]
    cfgs.define("LINUX", Some("1")).define("CEF_X11", Some("1"));

    #[cfg(target_os = "macos")]
    cfgs.define("MACOS", Some("1"));

    cfgs.compile("sys");

    #[cfg(target_os = "windows")]
    {
        println!("cargo:rustc-link-lib=libcef");
        println!("cargo:rustc-link-lib=libcef_dll_wrapper");
        println!("cargo:rustc-link-lib=delayimp");
        println!("cargo:rustc-link-lib=winmm");
        println!("cargo:rustc-link-arg=/NODEFAULTLIB:libcmt.lib")
    }

    #[cfg(target_os = "linux")]
    {
        println!("cargo:rustc-link-lib=cef");
        println!("cargo:rustc-link-lib=cef_dll_wrapper");
        println!("cargo:rustc-link-lib=X11");
    }

    #[cfg(target_os = "macos")]
    {
        println!("cargo:rustc-link-lib=framework=Chromium Embedded Framework");
    }

    println!("cargo:rustc-link-lib=static=sys");
    println!("cargo:rustc-link-search=all={}", out_dir.display());
    println!("cargo:rustc-link-search=all={}/Release", cef_path.display());

    #[cfg(not(target_os = "macos"))]
    println!(
        "cargo:rustc-link-search=all={}/libcef_dll_wrapper/Release",
        cef_path.display(),
    );

    Ok(())
}
