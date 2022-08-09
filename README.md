# HeadphoneDAC
This project started with the purchase of a couple of Chinese DAC modules designed around a PCM1794 DAC from TI. It turned out the DAC module was of quite good quality with genuine parts and a design closely following the datasheet reference design for the analog part. I therefore decided to make a headphone amplifier out of this with USB input. For this reason I had to design and construct the actual headphone amplifier as well as the logic that converts the USB data into an I2S stream that can be read by the DAC. A suitable power supply also had to be designed. All in all, the whole project therefore consists of 4 modules, one logic board with a STM32F411 running the show, one DAC module, one headphone amplifier module and one power supply.

The logic module takes care of converting the USB audio stream into I2S data on a USB full-speed interface. This sets a limit of the maximum sample rate to 96 kHz. It implements the proper way to handle the audio data, i.e., asynchronously. As of now, the system clock of the STM32 microcontroller is used for the audio clock, something that can be improved in the future. The STM32F411 microcontroller also takes care of the user interface, which consists of a few buttons and LEDs to indicate playback, mute and sampling rate. A LED VU meter is also implemented as well as control of a relay to avoid pops and clicks at startup and shutdown.

The headphone amplifier is a class A output design, using a NE5534 opamp as voltage amplifier and an emitter follower implemented as a Sziklai pair with a current sink. The design itself has borrowed from two sources, the Youtube channel Innove Workshop (but modified for dual supply) and Elliott Sound Products (https://sound-au.com/project113.htm). It is a relatively simple design, but measures well (I get THD figures of approximately 0.03% at 1 kHz and 100 mW, which is around the limit of my home made sound card using a Raspberry Pi and Hifiberry ADC+DAC). 

The power supply is a simple design giving +/-12 V and +5V, with a toroidal transformer and LM317/337 and L7812/7809/7805.

I am pretty happy with this build and think it sounds really good with my Beyerdynamic DT880 Pro headphones (250 ohms). I had some problems with ground loops and interference from the transformer in the case I built for the amplifier, but I think I have sorted out most of that now with some tweaking of the orientation of the transformer as well as appropriate grounding. What remains is an irritating high-pitched interference obviously coming from the digital parts of the controller module, which oddly enough can only be heard when muting the sound (if streaming is on, but no music is playing or very low levels of music, I can't hear it).
<p align="center">
  <img src="images/imge.jpg" width="350" title="Prototype">
</p>
