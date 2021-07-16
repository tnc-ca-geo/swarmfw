### Unit tests ###

There are great unit testing libraries for Arduino out there. For now we are 
using https://github.com/bxparks/AUnit. However, testing on Arduino is still a somewhat 
sketchy topic (big hopes for Arduino 2).

A big problem for testing is how Arduino handles directories in their build process. They 
analyze the user directory tree and copy files to a build directory. The downside is 
that imports from relative paths only work if the path is a child path of the one where 
the .ino file is placed. Even worse we can not have a test.ino file in the same directory
as the project .ino. In short, the project structure of this project does not really work
for unit tests. We are HACKING it by creating a symlink to the /src within the test sketch.
Git plays well with that, however it need to be changed in a Windows environment.

Another interesting idea, I found is that development could be vastly accelerated if we 
could run tests for Arduino unspecific tasks, i.e. extracting values from strings, in a
local C++ environment rather than on Arduino hardware. This would also open up the 
opportunity to develop parts of the project without access to hardware.

A recommended way to deal with this problems is to develop libraries in the Arduino library 
directory. Another is to create your own build process. That all requires more expertise 
of the C++ ecosystem I have. So we leave the hacky way for now.

Please appreciate that I think of unit testing at all.

Just in case it is not obvious. For testing upload the .ino sketches in the tests directory
to a compatible device and observe the Serial output.
