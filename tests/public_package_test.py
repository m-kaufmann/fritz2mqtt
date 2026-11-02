"""Offline boundaries and consistency checks for the standalone source package."""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMPONENT = ROOT / 'components/fritzbox_aha'

class PublicPackage(unittest.TestCase):
    def test_no_private_application_or_device_archives(self):
        for name in ['Integration', 'local_devices', 'heizung_include', 'joomla_extensions', 'work']:
            self.assertFalse((ROOT / name).exists(), name)
        for folder in ['components', 'firmware', 'packages', 'hardware']:
            for path in (ROOT / folder).rglob('*'):
                self.assertNotIn(path.suffix, ['.heic', '.php', '.sql', '.pem', '.key', '.bin'])

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

    def test_page_and_topic_suggestions(self):
        page = (COMPONENT / 'gateway.html').read_text()
        embedded = (COMPONENT / 'gateway_page.h').read_text().split('R"F2M(\n', 1)[1].split('\n)F2M";', 1)[0]
        self.assertEqual(page, embedded)
        self.assertIn("'fritz2mqtt/'+config.location", page)
        self.assertIn("prefix+d.base+'/set'", page)
        self.assertIn('FRITZ2MQTT Gateway', page)

if __name__ == '__main__':
    unittest.main()
