# Movement Exhibition

This demo project was born to experiment different kinds of movement mechanics and practice math applied to kinematics.
As a side effect, I ended up extending the Character Movement Component provided by Unreal Engine.
All the mechanics implemented are fully replicated and tested for multiplayer.

I used the setup from the great tutorial made by [@delgoodie](https://www.youtube.com/playlist?list=PLXJlkahwiwPmeABEhjwIALvxRSZkzoQpk) and then I proceeded implementing stuff on my own. 

# Gameplay Overview

[Showcase video](https://youtu.be/SoE8ifFN9mw)

# Mechanics

* **Hook**
* **Rope**
* **Dive**
* **Sliding**

### Hook
Your character is equipped with a **grappling hook** so now you can move through the map using this fast-paced gadget. 
While you hold the button you go towards your destination but you can also leave it to just gain velocity in air and make some unexpected move :)

* You can tune how fast the hook should be and you can specify a curve defining it. 
* You can decide to handle or not a cable (`UCableComponent`) to simulate grappling hook visually.
* You can grapple to everything as long as it has the right **Tag**, which is tunable.
* You can specify how much **velocity** you **lose** after you reach your **destination** or you leave it mid-air (in percentage).
* You can enter a **montage** to animate the enter into **hook state**.

[Showcase video](https://youtu.be/1bo_Xaxb08E)

### Rope
You can attach to a **rope** and follow it until you reach the other end of it... or leave it earlier to surprise your enemy.

* This feature **overrides** the **jump** mechanic and **search** for a **rope**. If it doesn't find a rope you jump as usual.
* The **destination** is automatically **calculated**. 
* The player gets automatically **adjusted in the center of the rope**.
* You can adjust **how** much **high** you want to check for the **rope** (simulating a jump).
* You can **adjust** the position of the capsule **under the rope** (in percentage relative to capsule half height).
* You can tune how **fast** you want to **transition** to the **rope**.
* As for Hook, **everything** can be considered as a **rope** as long as it has the right **Tag**, which is tunable.
* As for Hook, you can tune the **max speed** and you can also define a **curve** for it.
* You can specify a **montage** to **jump** towards the rope.

[Showcase video](https://youtu.be/8U-bufRi79E)

### Dive
I implemented my version of the classical "**roll**" of souls-like games.
If you have enough velocity you go in a **dive roll**, otherwise you do a **back-dodge**.
If you're in air you gain a little boost in velocity simulating a **dash**.

[Showcase video](https://youtu.be/Uoa1Xy0DO7I)

### Sliding
Using the **shift** button you can slide or crouch based on velocity. This limit is tunable.
Sliding is physics based so you gain or lose velocity on ramps depending which direction you're going.
You can also tune the braking deceleration speed.

[Showcase video](https://youtu.be/7Rf7_f4hExY)

### Multiplayer demonstration
In this video I briefly show all the mechanics implemented in a multiplayer environment with a listen-server and a client.

[Showcase video](https://www.youtube.com/watch?v=wUncXO2YpuU)

### Future implementations
* Climbing ladders
* Vaulting
* Moving in cover
