from distutils.core import setup
setup(name='MGCspendfrom',
      version='1.0',
      description='Command-line utility for massgrid "coin control"',
      author='Gavin Andresen',
      author_email='gavin@massgridfoundation.org',
      requires=['jsonrpc'],
      scripts=['spendfrom.py'],
      )
