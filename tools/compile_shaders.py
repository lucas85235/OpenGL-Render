#!/usr/bin/env python3
"""
Unified Shader Compiler
Processes .shader files into SPIR-V binaries for Vulkan/OpenGL.

Usage:
    python compile_shaders.py <input_dir> <output_dir> [--api vulkan|opengl] [--version 450]

Example:
    python compile_shaders.py src/shaders build/shaders --api vulkan --version 450
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

@dataclass
class ShaderStage:
    name: str
    extension: str
    glsl_extension: str
    glsl_stage: str  # For glslangValidator -S option

STAGES = {
    'vertex': ShaderStage('vertex', '.vert.spv', '.vert', 'vert'),
    'fragment': ShaderStage('fragment', '.frag.spv', '.frag', 'frag'),
    'geometry': ShaderStage('geometry', '.geom.spv', '.geom', 'geom'),
    'compute': ShaderStage('compute', '.comp.spv', '.comp', 'comp'),
    'tess_control': ShaderStage('tess_control', '.tesc.spv', '.tesc', 'tesc'),
    'tess_eval': ShaderStage('tess_eval', '.tese.spv', '.tese', 'tese'),
}

def extract_block(source: str, block_name: str) -> str:
    """Extract content between #pragma begin(name) and #pragma end(name)."""
    pattern = rf'#pragma\s+begin\s*\(\s*{block_name}\s*\)(.*?)#pragma\s+end\s*\(\s*{block_name}\s*\)'
    match = re.search(pattern, source, re.DOTALL)
    return match.group(1).strip() if match else ''

def transform_bindings(resources: str, api: str, version: int) -> str:
    """Transform @binding annotations to API-specific syntax."""
    if not resources:
        return ''
    
    result = resources
    
    if api == 'vulkan':
        # @binding(N) @set(M) -> layout(set=M, binding=N)
        result = re.sub(
            r'@binding\s*\(\s*(\d+)\s*\)\s*@set\s*\(\s*(\d+)\s*\)',
            r'layout(set = \2, binding = \1)',
            result
        )
        # @binding(N) -> layout(set=0, binding=N)
        result = re.sub(
            r'@binding\s*\(\s*(\d+)\s*\)',
            r'layout(set = 0, binding = \1)',
            result
        )
        # @push_constant -> layout(push_constant)
        result = re.sub(r'@push_constant', 'layout(push_constant)', result)
        # @readonly -> readonly
        result = re.sub(r'@readonly', 'readonly', result)
    else:
        # OpenGL
        if version >= 420:
            result = re.sub(
                r'@binding\s*\(\s*(\d+)\s*\)(?:\s*@set\s*\(\s*\d+\s*\))?',
                r'layout(binding = \1)',
                result
            )
        else:
            result = re.sub(
                r'@binding\s*\(\s*\d+\s*\)(?:\s*@set\s*\(\s*\d+\s*\))?\s*',
                '',
                result
            )
        result = re.sub(r'@push_constant\s*', '', result)
        result = re.sub(r'@readonly\s*', '', result)
    
    return result

def generate_stage_glsl(version: int, common: str, resources: str, stage_code: str) -> str:
    """Assemble a complete GLSL file for a stage."""
    lines = [f'#version {version} core', '']
    
    if common:
        lines.append('// ===== COMMON =====')
        lines.append(common)
        lines.append('')
    
    if resources:
        lines.append('// ===== RESOURCES =====')
        lines.append(resources)
        lines.append('')
    
    lines.append('// ===== STAGE =====')
    lines.append(stage_code)
    
    return '\n'.join(lines)

def process_shader_file(shader_path: Path, output_dir: Path, api: str, version: int) -> bool:
    """Process a .shader file and generate SPIR-V binaries."""
    print(f'[Shader] Processing: {shader_path.name}')
    
    source = shader_path.read_text()
    
    common = extract_block(source, 'common')
    resources = extract_block(source, 'resources')
    transformed_resources = transform_bindings(resources, api, version)
    
    shader_name = shader_path.stem
    success = True
    
    for stage_name, stage_info in STAGES.items():
        stage_code = extract_block(source, stage_name)
        if not stage_code:
            continue
        
        glsl_source = generate_stage_glsl(version, common, transformed_resources, stage_code)
        
        # Write temporary GLSL file
        temp_glsl = output_dir / f'{shader_name}{stage_info.glsl_extension}'
        temp_glsl.write_text(glsl_source)
        
        # Compile to SPIR-V
        spv_output = output_dir / f'{shader_name}{stage_info.extension}'
        
        cmd = [
            'glslangValidator',
            '-V',  # Generate SPIR-V
            '-S', stage_info.glsl_stage,
            '-o', str(spv_output),
            str(temp_glsl)
        ]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                error_msg = result.stdout + result.stderr
                print(f'  [ERROR] {stage_name}:\n{error_msg.strip()}')
                success = False
            else:
                print(f'  [OK] {stage_name} -> {spv_output.name}')
        except FileNotFoundError:
            print('[ERROR] glslangValidator not found. Install via: apt install glslang-tools')
            return False
    
    return success

def main():
    parser = argparse.ArgumentParser(description='Compile unified .shader files to SPIR-V')
    parser.add_argument('input_dir', help='Directory containing .shader files')
    parser.add_argument('output_dir', help='Output directory for SPIR-V binaries')
    parser.add_argument('--api', choices=['vulkan', 'opengl'], default='vulkan',
                        help='Target API (default: vulkan)')
    parser.add_argument('--version', type=int, default=450,
                        help='GLSL version (default: 450)')
    parser.add_argument('--keep-glsl', action='store_true',
                        help='Keep intermediate GLSL files')
    
    args = parser.parse_args()
    
    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    
    if not input_dir.exists():
        print(f'[ERROR] Input directory not found: {input_dir}')
        sys.exit(1)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    shader_files = list(input_dir.glob('*.shader'))
    if not shader_files:
        print(f'[WARN] No .shader files found in {input_dir}')
        sys.exit(0)
    
    print(f'[Shader] Compiling {len(shader_files)} shader(s) for {args.api.upper()} (GLSL {args.version})')
    print()
    
    all_success = True
    for shader_file in shader_files:
        if not process_shader_file(shader_file, output_dir, args.api, args.version):
            all_success = False
    
    # Clean up intermediate GLSL files unless --keep-glsl
    if not args.keep_glsl:
        for ext in ['.vert', '.frag', '.geom', '.comp', '.tesc', '.tese']:
            for temp_file in output_dir.glob(f'*{ext}'):
                temp_file.unlink()
    
    print()
    if all_success:
        print('[Shader] All shaders compiled successfully!')
    else:
        print('[Shader] Some shaders failed to compile.')
        sys.exit(1)

if __name__ == '__main__':
    main()
