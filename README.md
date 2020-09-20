# XyRegEngine
## Features
- Support most ECMAScript syntax except:
    - \c \u \x
    - POSIX-like classes
- Now it only supports ascii encoding for object strings.
- Support two mode: 
    - search -- try to find a sub-string satisfying the regex
    - match -- match the whole string with the regex
## Getting started
- get the code:
`git clone https://github.com/ddxy18/XyRegEngine.git`
- download googletest to ./GoogleTest
- build the project with cmake