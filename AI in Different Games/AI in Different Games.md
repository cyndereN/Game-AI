# AI in Different Games

##  0. General Intelligence in Games

AI is not good at it. AI is good at solving problems for specific domains.

## 1. F.E.A.R. - Goal Oriented Action Planning

![](assets/Fear.png)

## 2. Arkham Asylum - FSM

![](assets/ArkhamAsylum.png)

## 3. Pac-man

## 4. L4D - Director of AI

Manages the flow of gameplay, like intensity.

Manipulates the curretn state of the gmae world and an effort to keep players under duress

- The build-up
- The peak
- The relax

Stress levels increase by:
- Zombies attacking players (gradual)

- Attacks within proximity(gradual)

- Special attacking players (instant)

## 5. HALO2 - Behaviour Tree

![](assets/HALO2.png)

Think in context

Priority in context (some times if in vehicle, enemy will not hide, instead it attacks and sometimes get closer)

## 6. Battlefield 3 - Performance Modelling

e.g., drivatar in Forza, director of L4D

Psy-Ops, Anylize player

The Big Five:
- Openness
- Conscientiousness
- Extroversion
- Agreeableness
- Neuroticism

## 7. Transformers: Fall of Cybertron - HTN Planning

Focus on abstract behaviour by structuring actions together

## 8. Alien: Isolation - 100-Node BTs, Micro AIs

Scarier when closer but 

- Two Tier System 

    - Director: Managers the scene and keeps tabs on the current status of both the player and the Alien

    - The Alien AI: A reactive sense-driven NPC that reacts both to player actions and commands from the director

- Job System
    
    Dictates to Alien AI

    - What tasks to execute
    - The locations to conduct these tasks
    - The priority of each task

- Two Main Alien AI States

    - Active / Front State

        Conduct a search in th local area of the player or an event

    - Passive / Back State

        Alien climbs up into the air vents or directed to other areas of the station

- BTs

    - 30 nodes wide (Top decision layer)
        
        Sub-trees dedicated to specific tasks (attacking, searching, etc.)

    - Learn-rate can vary

        In the event the player has not caused specific sub-trees to unlock by their own action, the system will unlock them as specific checkpoints

- "Sensor" system

    - Variety of path-findin-driven sensors
        
        Alien can detect a variety of noises from footsteps to gunshots and the motion tracker. Range of each sensor depends on the type of noise being made

    - Pathifinding location types

        Search: Moves towards that location

        Spot: Stays wherre it's a standing and looks towards it

        Designers can dictate areas it cannot visit

- Searching System

    - Sub-optimal Exploration of all Nav Points: Alien searches all prescribed locations but in any given sequence, sometimes resulting in the alien backtracking to areas it visited before