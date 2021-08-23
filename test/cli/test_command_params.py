import unittest
import click.testing

class TestParamsCommand(unittest.TestCase):

    def setUp(self):
        self.cli = click.testing.CliRunner()

    def test_nothing(self):
        self.assertTrue(1 == 1)

if __name__ == '__main__':
    unittest.main()