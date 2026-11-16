"""Offline boundaries and consistency checks for the standalone source package."""
import re
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPONENT = ROOT / 'components/fritzbox_aha'

class PublicPackage(unittest.TestCase):
    def test_no_private_application_or_device_archives(self):
        for name in ['Integration', 'local_devices', 'heizung_include', 'joomla_extensions', 'work']:
            self.assertFalse((ROOT / name).exists(), name)
        # Check the actual publication set, including forced-added ignored files.
        # Locally generated, ignored firmware files are intentionally not published.
        paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard'], cwd=ROOT, text=True).splitlines()
        for name in paths:
            self.assertNotIn(Path(name).suffix, ['.heic', '.php', '.sql', '.pem', '.key', '.bin'])

    def test_factory_is_unconfigured(self):
        source = (COMPONENT / '__init__.py').read_text()
        self.assertIn('cv.Optional(CONF_USERNAME, default="")', source)
        self.assertIn('cv.Optional(CONF_PASSWORD, default="")', source)
        self.assertIn('cv.Optional("host", default="0.0.0.0")', source)
        self.assertIn('default="fritz2mqtt"', source)
        config = (ROOT / 'firmware/esp32.yaml').read_text()
        self.assertNotIn('!secret', config)
        self.assertNotRegex(config, re.compile(r'^\s*(password|key|username):', re.MULTILINE))
        self.assertNotIn('api:', config)
        self.assertIn('flash_size: 4MB', config)
        self.assertIn('name_add_mac_suffix: true', config)

    def test_configurable_storage_retains_private_installations(self):
        source = (COMPONENT / 'gateway_web.cpp').read_text()
        self.assertEqual(source.count('nvs_open(storage_namespace_.c_str(),'), 2)
        self.assertIn('"config_v1"', source)
        self.assertIn('set_storage_namespace', (COMPONENT / 'fritzbox_aha.h').read_text())

    def test_separate_test_channel(self):
        test_profile = (ROOT/'firmware/esp32-test.yaml').read_text()
        self.assertIn('/testing/manifest.json', test_profile)
        self.assertIn('firmware_channel: testing', test_profile)
        stable = (ROOT/'firmware/esp32.yaml').read_text()
        self.assertIn('/stable/manifest.json', stable)
        self.assertNotIn('/testing/', stable)

    def test_page_and_topic_suggestions(self):
        page = (COMPONENT / 'gateway.html').read_text()
        embedded = (COMPONENT / 'gateway_page.h').read_text().split('R"F2M(\n', 1)[1].split('\n)F2M";', 1)[0]
        self.assertEqual(page, embedded)
        self.assertIn("namespace+'/'+config.location", page)
        self.assertIn("config.mqtt.topic_prefix", page)
        self.assertIn("['command_topic','Neuer-SOLL-Topic','/set']", page)
        self.assertIn('FRITZ2MQTT Gateway', page)

    def test_embedded_icons(self):
        encoded = (COMPONENT / 'gateway_icon.h').read_text()
        page = (COMPONENT / 'gateway.html').read_text()
        for filename, symbol, link_type, sizes in [
            ('apple-touch-icon.png', 'GATEWAY_TOUCH_ICON', 'apple-touch-icon', '180x180'),
            ('favicon.ico', 'GATEWAY_FAVICON', 'icon', '16x16 32x32'),
        ]:
            with self.subTest(filename=filename):
                icon = (COMPONENT / filename).read_bytes()
                if filename.endswith('.png'):
                    self.assertEqual(icon[:8], b'\x89PNG\r\n\x1a\n')
                    self.assertEqual(icon[16:24], b'\0\0\0\xb4\0\0\0\xb4')
                else:
                    self.assertEqual(icon[:6], b'\0\0\1\0\2\0')
                    self.assertEqual(icon[6:8], bytes([16, 16]))
                    self.assertEqual(icon[22:24], bytes([32, 32]))
                array = encoded.split(f'{symbol}[] = {{', 1)[1].split('};', 1)[0]
                self.assertEqual(bytes(int(value, 16) for value in re.findall(r'0x([a-f0-9]{2})', array)), icon)
                self.assertRegex(page, rf'rel="{link_type}"[^>]*sizes="{sizes}" href="/{filename}"')

if __name__ == '__main__':
    unittest.main()
