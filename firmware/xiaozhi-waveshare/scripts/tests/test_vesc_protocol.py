import pathlib
import os
import shutil
import subprocess
import tempfile
import unittest


class VescProtocolTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which('g++'), 'native g++ is needed for protocol tests')
    def test_protocol_with_sanitizers(self):
        root = pathlib.Path(__file__).resolve().parents[2]
        board = root / 'main/boards/waveshare/esp32-s3-touch-lcd-3.5'
        cjson = root / 'managed_components/espressif__cjson/cJSON'
        if not cjson.exists():
            # Windows source-only bundles intentionally omit managed dependencies.
            self.skipTest('run after idf.py reconfigure has resolved cJSON')
        with tempfile.TemporaryDirectory(prefix='vesc-host-') as directory:
            binary = str(pathlib.Path(directory) / 'vesc-test')
            cjson_object = str(pathlib.Path(directory) / 'cjson.o')
            subprocess.run(['gcc', '-c', str(cjson / 'cJSON.c'), '-o', cjson_object,
                            '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'],
                           check=True, capture_output=True, text=True)
            subprocess.run([
                'g++', '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                '-I', str(board), str(board / 'vesc_protocol.cc'),
                '-I', str(cjson), str(board / 'vesc_status.cc'), cjson_object,
                str(root / 'scripts/tests/vesc_protocol_test.cpp'), '-o', binary,
            ], check=True, capture_output=True, text=True)
            result = subprocess.run([binary], check=True, capture_output=True, text=True,
                                    env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=1'})
            self.assertIn('PASS:', result.stdout)


if __name__ == '__main__':
    unittest.main()
