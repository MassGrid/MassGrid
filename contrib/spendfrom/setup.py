from distutils.core import setup
setup(name='MLGBspendfrom',
      version='1.0',
      description='Command-line utility for mlgbcoin "coin control"',
      author='Gavin Andresen',
      author_email='gavin@mlgbcoinfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
