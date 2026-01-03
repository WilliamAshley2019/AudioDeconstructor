OK this is doing what I set out to do with it - explainer:

So the idea is making a plugin you can load an audio file into (currently .wav as the test format)  then taking the breakpoint file as a mechanism to do things like presets and automation etc..  we extract various audio 
characteristics from the audio file and have them able to be saved and applied in other plugins. So say I want to mirror a specific aspect of that audio file, i can map the breakpoints of that file then apply it via the 
effector plugin for that file. Now its still sort of basic but the concept is working. I still need to work on the effector plugiuns that go in the set for each audio characteristic.

This plugiun just pulls some data from the audio file and allows it to be saved as the breakpoint file. 


Built with Juce 8.0.12
Using plugin basics and dsp module.

This uses a few extra files that need to be added to the build as I usually try to get everything into the plugineditor and pluginprocessor .h and .cpp files. 
This one was a little more complex and novel for me, building off the uber panner essentials concept of buildilng a breakpoint pan map. 

Then I was like well maybe I can do like a vst3 of Music N or V but modernized. So I am like well what if we extract audio character and then reapply that like if you can synthesize a flute then why can't you apply the 
character of a flute to another type of audio via a vst3 effect process??? 

So this is still very proto but I took a few characteristics and set out to extract those aspects. 
Atleast in principle this is working but  I think the gui actually needs to be "refined" to allow some linearlization of the breakpoints as the sheer number of points and their display is a little unweildy. 
Its not exactly a wave editor but I think modfifying those breakpints could in fact work as a specialized wave editor but still very primitive.

But I am sort of trying to implent very old audio processes for audio manipualtion but rejuvinate that into a modernized vst3 plugins format
