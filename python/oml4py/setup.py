from distutils.core import setup
setup(name='oml4py',
      version='1.0.0',
      author = "Fraida Fund",
      author_email = "ffund01@students.poly.edu",
      description = ("An OML client module for Python"),
      url = "http://witestlab.poly.edu",
      download_url = "http://pypi.python.org/pypi/oml4py",
      py_modules=['oml4py'],
      license = "MIT",
      classifiers=[
          'License :: OSI Approved :: MIT License',
          'Operating System :: OS Independent',
          'Topic :: Scientific/Engineering',
      ],
      long_description=open('README.txt', 'rt').read(),
      )
