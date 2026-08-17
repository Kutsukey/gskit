import struct
import math

def write_ply(filename):
    properties = [
        "x", "y", "z",
        "scale_0", "scale_1", "scale_2",
        "rot_0", "rot_1", "rot_2", "rot_3",
        "opacity",
        "f_dc_0", "f_dc_1", "f_dc_2"
    ]
    
    vertices = [
        [0.0, 0.0, 0.0, -5.0, -5.0, -5.0, 1.0, 0.0, 0.0, 0.0, -2.0, 0.5, 0.5, 0.5],
        [float('nan'), 0.0, 0.0, -5.0, -5.0, -5.0, 1.0, 0.0, 0.0, 0.0, -2.0, 0.5, 0.5, 0.5],
        [0.0, 0.0, 0.0, -50.0, -5.0, -5.0, 1.0, 0.0, 0.0, 0.0, -2.0, 0.5, 0.5, 0.5],
        [0.0, 0.0, 0.0, -5.0, -5.0, -5.0, 2.0, 0.0, 0.0, 0.0, -2.0, 0.5, 0.5, 0.5],
        [0.0, 0.0, 0.0, -5.0, -5.0, -5.0, 1.0, 0.0, 0.0, 0.0, float('inf'), 0.5, 0.5, 0.5],
    ]
    
    with open(filename, 'wb') as f:
        header = "ply\n"
        header += "format binary_little_endian 1.0\n"
        header += f"element vertex {len(vertices)}\n"
        for prop in properties:
            header += f"property float {prop}\n"
        header += "end_header\n"
        f.write(header.encode('ascii'))
        
        for vertex in vertices:
            for value in vertex:
                f.write(struct.pack('<f', value))
    
    print(f"Generated {filename} with {len(vertices)} vertices")

if __name__ == "__main__":
    write_ply("corrupt.ply")