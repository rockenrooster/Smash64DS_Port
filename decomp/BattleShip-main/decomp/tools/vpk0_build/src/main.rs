use std::env;
use std::io;
use std::io::prelude::*;
use std::fs::read_to_string;
use std::fs::File;
use std::process;
use vpk0::vpk_info;
use vpk0::{Encoder};
use vpk0::format::{VpkMethod};
use std::path::{PathBuf};

fn main() -> io::Result<()>
{
	let args: Vec<String> = env::args().collect();

	let mut args_validated = false;
	args_validated |= args.len() == 4 && (args[1] == "c" || args[1] == "d");
	args_validated |= args.len() == 3 && (args[1] == "i");

	if !args_validated {
		println!("Usage:");
		println!("  Compress:   vpk0 c in_file_path out_file_path");
		println!("  Decompress: vpk0 d in_file_path out_file_path");
		println!("  Get info:   vpk0 i in_file_path");
		process::exit(1);
	}

	let mut input_file = File::open(&args[2])?;
	let mut input_buffer = Vec::new();
	input_file.read_to_end(&mut input_buffer)?;
	input_file.seek(std::io::SeekFrom::Start(0))?;

	match args[1].as_str() {
		"c" => {
			let mut config_file_path = PathBuf::from(&args[2]);
			config_file_path.set_extension("vpk0_config");
			if config_file_path.as_path().exists() {

				// custom compression
				let file_content = read_to_string(config_file_path.as_path()).unwrap();
				let lines: Vec<&str> = file_content.lines().collect();

				let method_string = String::from(lines[0]);
				let offsets = String::from(lines[1]);
				let lengths = String::from(lines[2]);

				let method: VpkMethod = unsafe { std::mem::transmute(method_string.parse::<i32>().unwrap() as i8) };

				let output_buffer = Encoder::for_bytes(input_buffer.as_slice())
					.method(method)
					.optional_offsets(Some(&offsets))
					.optional_lengths(Some(&lengths))
					.encode_to_vec();

				let mut file = File::create(&args[3])?;
				file.write_all(output_buffer.unwrap().as_slice())?;
			}
			else {

				// default compression
				let output_buffer: Vec<u8>;
				output_buffer = vpk0::encode_bytes(input_buffer.as_slice()).unwrap();
				let mut file = File::create(&args[3])?;
				file.write_all(output_buffer.as_slice())?;
			}
		},
		"d" => {
			// decompress
			let output_buffer: Vec<u8>;
			output_buffer = vpk0::decode_bytes(input_buffer.as_slice()).unwrap();
			let mut decompressed_file = File::create(&args[3])?;
			decompressed_file.write_all(output_buffer.as_slice())?;

			// write compression info
			let (header, trees) = vpk_info(input_file).unwrap();
			let mut config_file_path = PathBuf::from(&args[3]);
			config_file_path.set_extension("vpk0_config");
			let mut config_file = File::create(&config_file_path.as_path())?;
			let _ = config_file.write_all(format!("{}\n{}\n{}", header.method as u32, trees.offsets, trees.lengths).as_bytes());
		}
		"i" => {
			let (header, trees) = vpk_info(input_file).unwrap();
			println!("Original size: {} bytes", header.size);
			println!("VPK encoded with method {}", header.method);
			println!("Tree offsets: {}", trees.offsets);
			println!("Tree lengths: {}", trees.lengths);
		}
		&_ => {} // won't happen
	}

	Ok(())
}