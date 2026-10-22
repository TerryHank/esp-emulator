from pathlib import Path
import importlib.util
import subprocess
import re

task=Path(__file__).resolve().parent / 'carcyd_fonts'
workspace=Path(__file__).resolve().parents[1]
b=workspace/'main/boards/waveshare/esp32-s3-touch-lcd-3.5'
spec=importlib.util.spec_from_file_location('original_fonts',task/'generate_fonts_upstream.py')
g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
g.ROOT=workspace
g.TTF_DIR=task/'ttf'
g.LV_SYMBOL_REMAP.append((0xF55A,0xF004D)) # Keyboard backspace uses the original back-arrow glyph.
g.OUT_DIR=b/'carcyd/ui/fonts'
sizes={12:16,14:18,16:20,20:28,28:36,64:88,24:32,48:56}
g.FONTS=[(name,sizes[size],ttf,text,syms,icons) for name,size,ttf,text,syms,icons in g.FONTS]
import shutil
converter=[shutil.which('node'),str(task/'node_modules/lv_font_conv/lv_font_conv.js')]
for font in g.FONTS:
    print(font[0],font[1],flush=True)
    subprocess.run(converter+g.build_args(*font),cwd=workspace,check=True,stdout=subprocess.DEVNULL)
g.write_fonts_header();g.write_icons_header()
text='\n'.join(p.read_text(encoding='utf-8') for p in b.rglob('*') if p.suffix in ('.h','.cc','.cpp') and 'fonts' not in p.parts)
symbols=''.join(sorted(set(re.findall(r'[\u4e00-\u9fff\u3000-\u303f\uff00-\uffef…℃·]',text))))
(task/'glyphs.txt').write_text(symbols,encoding='utf-8')
for old in (12,14,16,20,28):
    subprocess.run(converter+['--font',str(task/'ttf/NotoSansSC-Medium.ttf'),
        '--size',str(sizes[old]),'--bpp','4','--format','lvgl','--symbols',symbols,
        '--no-compress','--no-prefilter','--lv-font-name',f'carcyd_zh_{old}',
        '--lv-include','lvgl.h','-o',str(b/f'carcyd_zh_{old}.c')],check=True,stdout=subprocess.DEVNULL)
print('Native font sizes generated; Chinese glyphs:',len(symbols),flush=True)
