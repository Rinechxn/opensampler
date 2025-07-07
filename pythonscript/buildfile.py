import yaml
import struct
import os
import zlib
import argparse

def pack_string(s):
    b = s.encode('utf-8')
    return struct.pack('<H', len(b)) + b

def xor_encrypt(data: bytes, key: int = 0x5A) -> bytes:
    return bytes(b ^ key for b in data)

def build_osmp_embed_samples_compressed(folder_path, output_file, xor_key=0x5A):
    with open(os.path.join(folder_path, 'metadata.yaml'), 'r', encoding='utf-8') as f:
        metadata = yaml.safe_load(f)
    with open(os.path.join(folder_path, 'mapping.yaml'), 'r', encoding='utf-8') as f:
        mapping = yaml.safe_load(f)

    sample_data_list = []  # [(region_index, encrypted_bytes, original_size)]

    with open(output_file, 'wb') as f:
        f.write(b'OSMP')
        f.write(struct.pack('<I', 1))  # version 1

        metadata_yaml = yaml.dump(metadata, sort_keys=False)
        f.write(struct.pack('<I', len(metadata_yaml)))
        f.write(metadata_yaml.encode('utf-8'))

        global_info = mapping.get('global', {})
        global_volume = global_info.get('global_volume', 1)
        amp_veltrack = global_info.get('amp_veltrack', 0)
        f.write(struct.pack('<fI', float(global_volume), int(amp_veltrack)))

        groups = mapping.get('groups', [])
        f.write(struct.pack('<I', len(groups)))

        region_counter = 0
        region_sample_offsets = []

        for group in groups:
            name = group.get('name', 'Group')
            regions = group.get('regions', [])

            f.write(pack_string(name))
            f.write(struct.pack('<I', len(regions)))

            for region in regions:
                lokey = region.get('lokey', 0)
                hikey = region.get('hikey', 127)
                lovel = region.get('lovel', 0)
                hivel = region.get('hivel', 127)
                pitch_keycenter = region.get('pitch_keycenter', 60)
                volume = region.get('volume', 1.0)
                tune = region.get('tune', 0.0)

                f.write(struct.pack('<BBBBBff',
                    lokey, hikey, lovel, hivel, pitch_keycenter,
                    float(volume), float(tune)))

                sample_path = region.get('sample', '')
                sample_full_path = os.path.join(folder_path, sample_path)

                if os.path.isfile(sample_full_path):
                    with open(sample_full_path, 'rb') as sf:
                        sample_bytes = sf.read()
                    compressed_sample = zlib.compress(sample_bytes)
                    encrypted_sample = xor_encrypt(compressed_sample, xor_key)
                else:
                    print(f"⚠️ Warning: sample file not found: {sample_full_path}")
                    encrypted_sample = b''
                    sample_bytes = b''

                offset_pos = f.tell()
                f.write(struct.pack('<QII', 0, len(sample_bytes), len(compressed_sample)))
                region_sample_offsets.append((offset_pos, region_counter))
                sample_data_list.append(encrypted_sample)
                region_counter += 1

        sample_data_offset_start = f.tell()
        sample_offsets = []
        current_offset = sample_data_offset_start

        for encrypted_sample in sample_data_list:
            sample_offsets.append(current_offset)
            f.write(encrypted_sample)
            current_offset += len(encrypted_sample)

        for idx, (offset_pos, region_idx) in enumerate(region_sample_offsets):
            f.seek(offset_pos)
            f.write(struct.pack('<Q', sample_offsets[idx]))

    print(f"✅ Build complete! Output saved to: {output_file}")
    print(f"🔐 Encryption key: 0x{xor_key:02X} (Keep this for decryption in engine!)")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Build .osmp monolith file with embedded, compressed, encrypted samples.")
    parser.add_argument("input_folder", help="Path to the project folder (with mapping.yaml, metadata.yaml, and samples/)")
    parser.add_argument("output_file", help="Path to output .osmp file")
    parser.add_argument("--key", type=lambda x: int(x, 0), default=0x5A,
                        help="XOR encryption key (e.g., 0x3F or 123). Default is 0x5A")
    args = parser.parse_args()

    build_osmp_embed_samples_compressed(args.input_folder, args.output_file, xor_key=args.key)
