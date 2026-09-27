==========================
Taisei Headless Simulation
==========================

.. contents::

Introduction
------------

About Taisei Project
^^^^^^^^^^^^^^^^^^^^

Taisei Project is an open source fan-game set in the world of Touhou Project. It is a top-down vertical-scrolling curtain fire shooting game
(STG), also known as a “bullet hell” or “danmaku.” STGs are fast-paced games focused around pattern recognition and
mastery through practice.

About thrl Project
^^^^^^^^^^^^^^^^^^

`thrl <https://codeberg.org/thrl/thrl>`__, full name Touhou Reinforcement Learning Project, is the first Project support RL on Touhou 5.
Recently, I want to port Taisei to thrl project, so here it is.

About Headless Simulation
^^^^^^^^^^^^^^^^^^^^^^^^^
To be 100% accurate, a "headless simulation" is not the same definition as "Simulation" in Reinforcement Learning.
The different part is, here the word is referred to the process of modifying the game without the requirement of 
rendering. In RL, a Simulation is referred to a `software which emulates the environment <https://en.wikipedia.org/wiki/Emulator>`__,
makes the interaction easier.

Installation
------------
dummy string

Source Code & Development
-------------------------

Obtaining Source Code
^^^^^^^^^^^^^^^^^^^^^

Source
______

We recommend fetching the source code using ``git``:

.. code:: sh

   git clone --recurse-submodules https://github.com/touhourl/taisei-sim

Compiling Source Code
^^^^^^^^^^^^^^^^^^^^^

Currently, only GNU/Linux is supported. You could build by simply type: 
.. code:: sh

   make