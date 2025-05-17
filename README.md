# Wall Dash Trainer

A simple timer that shows how close you are to successfully performing wall dashes.

To perform wall dashes, you need to be able to double jump with roughly 90 ms or less between the jumps.
Works in freeplay and custom training.

This plugin was modified from the [Fast Aerial Trainer [Improved]](https://bakkesplugins.com/plugins/view/514) made by Vync and Josef37.


## Features

- Show time spent holding initial jump
- Show time between starting initial and double jump
- Timers are affected by freeplay game speed
- Customizable ranges for green, yellow and red portions of the timers
- Customizable GUI colors
- Customizable GUI position and scale

## Wall dashing

There are different methods for wall dashing. I have only tested this plugin with the following method:

1. Drive on a wall.
2. Hold the left analog stick forward.
3. Hold air roll right or left depending on which wall you are on. 
The point is to perform a diagonal flip towards the wall.
4. Tap jump twice as fast as you can.
5. Check [Speedometer](https://bakkesplugins.com/plugins/view/73) to verify that you are gaining speed.

## Practicing with slow motion

To make wall dashing easy, reduce freeplay speed to 50% - 90%. 

The game speed also affects the times shown by this plugin.
The timers show game time, not real time.

## Explanation of Values

### Hold First Jump

Duration jump button was held down on your initial jump.  
Faster is presumably better.

Holding the first jump down too long will not leave you any time to perform the second jump.

### Time Between Jumps

Time from first jump button press to second jump button press.  
This value includes the time spent holding down the initial jump.  
Faster is better.

Target is 80 ms or less. But values up to 95 ms has worked during testing.




## Changelog

See [CHANGELOG.md](./CHANGELOG.md).
