# Remote_door_chime
A remote chime rings when the door opens by means of a wireless transmission (2016-07)

This is the beginning of the story of how experiments with a simple wireless transmitter and receiver pair gave rise to a series of revisions that solved problems encountered along the way,

#### The problem
A concern for the saftey of an eldery family member created a need for knowing when the door to the outside was open.

#### The solution
Using a micro controller that sleeps the majority of time, a battery powered device is switched on every x ms and checks whether a sensor (Hall effect attached to the door frame) is in a particular state. If door is open, it sends a signal to a receiver located in another location. The receiver then plays a sound to indicate a door open signal has been received.

The approach used to manage sensor reading state changes and signal transmission has changed over several iterations of this project. 315MHz ASK was initially selected due to its simplicity and very low cost. The final version of this projected uses a nRF24L01+, due to its popularity and frequenncy range that is less prone to interfering with other devices.
