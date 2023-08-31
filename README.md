# sbuild
Very simple software renderer to draw perspective correct vertical walls, using fixed-point arithmetic only

## Initial public release commentary (31th of August, 2023)

When writing this playground, I wanted to get back to software rendering after playing DOOM (1993) and the fantastic Blood (1997). I already had made mitigated attempts with only succeeding at affine texture mapping a few years back, so I wanted to get back into the challenge!

The requirements is that the code should be able to run on CPUs without FPUs (so with fixed-point arithmetic only), and that it should be reasonably fast. I think I succeeded at the first at least, as for the second it could be further optimized, even if the fillrate is pretty good for a first naive attempt.

Here is a sample with a texture from the game Blood (1996). I believe this is in educational, no profit fair use. If you own any of this material and want this taken down, please contact me in the "Get in touch" section of my website: https://adrien-lenglet.fr/profession

![sbuild renderer showing a vertical wall in perspective from its corner](https://i.imgur.com/84kPHKq.png)

As a small disclaimer, this renderer does not perform z-clipping, only x and y clipping. So if any vertex goes behind the camera, funny things will happen ;)
