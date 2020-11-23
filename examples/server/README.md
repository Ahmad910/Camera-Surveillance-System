User manual
===========

Getting the project
---------------------

`git clone https://git.cs.lth.se/edaf55/C-camera.git`

`cd C-camera`

See the EDA040Project-CProblems.pdf for general helpful information.

The below instructions are intended to be run in the `C-camera/examples` directory.

Compilation on real camera
---------------------

You'll need the password to the camera from your supervisor. Cameras are available on argus-1 -- 10.

Run the following on a student computer (such as login.student.lth.se)


`make http_server`

`scp http_server rt@argus-7.student.lth.se:~/`

`ssh rt@argus-7`

`./http_server`

In a browser, go to

`http://argus-7.student.lth.se:9999`

Reload for new image


Compilation with fake camera
---------------------

`make -f Makefile.fake`

`./fake_server`

In a browser, go to

`localhost:9999`

Note: The first 86 images in the fake camera stream have no motion in them
