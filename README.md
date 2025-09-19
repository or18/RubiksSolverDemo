# RubiksSolverDemo
Online solver for Rubik's cube cross, xcross, free pair, last layer. Pseudo F2L solver and EOCross solver also available. Visual cube‑state editor is also available. <br>

[**Solver URL**](https://or18.github.io/RubiksSolverDemo/) 
<br>
[**Old version**](https://or18.github.io/Rubiks-cube-xcross-solver/)
<br>

Also, the following trainers are available. 
- [**Cross trainer**](https://or18.github.io/RubiksSolverDemo/cross_trainer)
- [**XCross trainer**](https://or18.github.io/RubiksSolverDemo/xcross_trainer)
- [**Free Pair trainer**](https://or18.github.io/RubiksSolverDemo/pairing_trainer)
- [**Pseudo XCross trainer**](https://or18.github.io/RubiksSolverDemo/pseudo_xcross_trainer)
- [**Pseudo Free Pair trainer**](https://or18.github.io/RubiksSolverDemo/pseudo_pairing_trainer)
- [**EOCross trainer**](https://or18.github.io/RubiksSolverDemo/eocross_trainer)



# **Solver Overview**
This is an online solver for speedcubers that uses specific methods such as CFOP and ZZ. It is designed to perform solution searches for techniques like cross, XCross, and EOCross. Additionally, it supports the search for some algorithms for the last layer. 

# **Basic Functions and Notes**

  - The following 13 solvers are currently available. 
  
    - **F2L Lite**: Lite solver for cross, XCross, XXCross, XXXCross, and XXXXCross. Analyzer available.

    - **Pairing**: Lite solver for a free pair. Analyzer available.

    - **Pseudo F2L Lite**: Lite solver for pseudo cross, XCross, XXCross, and XXXCross. Analyzer available.

    - **Pseudo Pairing**: Lite solver for a pseudo free pair. Analyzer available.

    - **EOCross**: Lite solver for EOCross, XEOCross, XXEOCross, XXXEOCross, and XXXXEOCross. Analyzer available.

    - **LL Substeps Lite**: Lite solver for last layer **CP**, **CO**, **EP**, and **EO**

    - **LL Lite**: Lite solver for last layer

    - **LL AUF Lite**: Lite solver for last layer and AUF

    - **Two Phase**: Two Phase solver using [min2phase.js](https://github.com/cs0x7f/min2phase.js)

    - **F2L**: Solver for cross, XCross, XXCross, XXXCross, and XXXXCross. **Recommended for use with PC**. Analyzer available.

    - **LL Substeps**: Solver for last layer **CP**, **CO**, **EP**, and **EO**. **Recommended for use with PC**.

    - **LL**: Solver for last layer. **Recommended for use with PC**.

    - **LL AUF**: Solver for last layer and AUF. **Recommended for use with PC**.

  - The last four solver, **F2L**, **LL Substeps**, **LL**, and **LL AUF**, can search much faster than their corresponding lite solver, but they require more memory and take longer to set up.

  -  Some solvers come with an analyzer that solves for each pattern one at a time and displays the shortest number of moves in HTM in a table. The numbered cells in this table act as buttons to start the search with that condition. You can click the header to sort the table by its columns. Also, you can select the faces to analyze from **U**, **D**, **L**, **R**, **F**, and **B**.

  - The following table shows the memory requirements for each solver. If memory is insufficient, the web page may crash (resulting in a forced reload). However, as mentioned below, it is possible to recover the page using the query parameter.

    | Solver | Single Search | Analyzer |
    |-----------|-----------|-----------|
    | **F2L Lite**   | 50 MB   | 50 MB   |
    | **Pairing**   | 50 MB   | 100 MB   |
    | **Pseudo F2L Lite**   | 50 MB   | 50 MB   |
    | **Pseudo Pairing**   | 50 MB   | 200 MB   |
    | **EOCross**   | 100 MB   | 100 MB   |
    | **LL Substeps Lite**   | 50 MB   | -   |
    | **LL Lite**   | 50 MB   | -   |
    | **LL AUF Lite**   | 50 MB   | -   |
    | **F2L**   | 800 MB   | 50 MB   |
    | **LL Substeps**   | 800 MB   | -   |
    | **LL**   | 800 MB   | -   |
    | **LL AUF**   | 800 MB   | -   |

- The scramble is saved in a temporary array each time **Scramble** is edited. You can navigate to the previous or next input in the history by pressing the **[⬅️]** or **[➡️]** button. If there is no previous or next history, the corresponding button will not appear.

- Pressing the **[Reverse]** button reverses the alg of each row and sorts them in reverse order. 

- Pressing the **[Mirror]** button mirrors the alg of each row.

- Pressing the **[Random]** button generates a random scramble using [min2phase.js](https://github.com/cs0x7f/min2phase.js). The generated scramble is automatically annotated with the comment by "// setup". 

- After configuring the appropriate settings, you can start the search by pressing the **[Start]** button. While the search is in progress, the **[Start]** button will change to an **[End]** button, which you can press to stop the search.

- Each time a solution is found, a Details section will be created that includes an **[Add]** button, a Twisty Player, and various links, with the output being updated in real time. By pressing the **[Add]** button, you can add the solution to the **Scramble**.

- Whenever the input fields are updated, the query parameters will also be updated. If a search is performed, the first solution will be added to the query parameters. 

- In rare cases, functions exported by Emscripten may not be initialized properly (especially on mobile devices). In such cases, a warning will be displayed, and you will be prompted to reload the page manually.

<br>

# **Input Description**
- **Cube Editor**: Visual scramble‑drawing system
  - In **Swap** mode, swaps two selected stickers, rotating the pieces so the stickers exchange positions.
  - In **Flip** mode, the selected piece’s orientation is rotated.
  - **[Get‑Scramble]** generates a scramble for the shown state (rotations aren’t included; edit if needed).
  - Shows “Unsolvable pattern.” alert when the state can’t be solved.

- **Scramble**: This describes the state of a Rubik's Cube using standard notations, and comments can also be added. 
  - The following Rubik's Cube notations are available:
    - **Face Turns:**
      - **U U2 U2' U'** 
      - **D D2 D2' D'**
      - **L L2 L2' L'**
      - **R R2 R2' R'**
      - **F F2 F2' F'**
      - **B B2 B2' B'**

    - **Wide Moves:**
      - **u u2 u2' u' Uw Uw2 Uw2' Uw'**
      - **d d2 d2' d' Dw Dw2 Dw2' Dw'**
      - **l l2 l2' l' Lw Lw2 Lw2' Lw'**
      - **r r2 r2' r' Rw Rw2 Rw2' Rw'**
      - **f f2 f2' f' Fw Fw2 Fw2' Fw'**
      - **b b2 b2' b' Bw Bw2 Bw2' Bw'**

    - **Slice Moves:**
      - **M M2 M2' M'**
      - **E E2 E2' E'**
      - **S S2 S2' S'**

    - **Rotations:**
      - **x x2 x2' x'**
      - **y y2 y2' y'**
      - **z z2 z2' z'**

  - Comments can be written using "//". It is recommended to place "// setup" comments on the appropriate line. When viewing on [alg.cubing.net](https://alg.cubing.net/) or [cubedb.net](https://cubedb.net/), the line with "// setup" and above will be included in the **Setup** or **Scramble** section, while the lines below will be included in the **Moves** section. 

- **Rotation**: Select a rotation alg before starting the search. 

- **Slot**: Select F2L slots from **BL** (**B**ack **L**eft), **BR** (**B**ack **R**ight), **FR** (**F**ront **R**ight), and **FL** (**F**ront **L**eft). If none of the four slots are selected, meaning **None** is chosen, it will function as cross solver.

- **Free Pair**: Select a slot for solving the free pair from the slots other than the one selected in the **Slot**.

- **Pseudo Slot Edge**: Select slot edges from **BL**, **BR**, **FR**, and **FL**. If none of the four slots are selected, meaning **None** is chosen, it will function as a **pseudo cross solver**, which solves cross when **D** face misalignment is allowed.

- **Pseudo Slot Corner**: Select slot edges from **BL**, **BR**, **FR**, and **FL**. Dropbox for this will appear when anything other than **None** is selected for **Pseudo Slot Edge**.

- **Free Pair Edge**: Select a pseudo slot edge for solving the pseudo free pair from the slots other than the one selected in the **Pseudo Slot Edge**. 

- **Free Pair Corner**: Select a pseudo slot corner for solving the pseudo free pair from the slots other than the one selected in the **Pseudo Slot Corner**. 

- **Last Layer Option**: Select from **CP**, **CO**, **EP**, and **EO**. If none of these options are selected, meaning **None** is chosen, it will function as XXXXCross solver.

- **Max Length**:  The maximum number of moves in HTM for the solution to be searched.

- **Max Count**: The maximum number of solutions to be searched.

- **Move restrict**: Restrictions on the moves used in the search. For example, if **U, R, F** is selected, the allowed moves are restricted to **U U2 U' R R2 R' F F2 F'**.  **Advanced settings** allow more detailed configuration. When you select Rotations (**x, y, z**), set **Center Restrict** and **Max Rotation Count** in the **Advanced Settings**.

<br>

# **Advanced Settings**

- **Pre Move**: A preliminary algorithm executed before search. This enables the search to solutions beginning with the algorithm set here. If you set algorithms here that rotate the center, such as wide, slice, or rotation, you must also configure **Center Restrict** accordingly as described below.

- **Move Available Table**: A table of checkboxes that lets you decide whether a particular ordering of moves—e.g., allowing the order A→B —is permitted in the solution. The rows represent preceding rotations, and the columns represent subsequent rotations. Therefore, enabling the checkbox in row A and column B activates the order A→B. The bottom row (A=empty) corresponds to the first move setting. This table is normally generated automatically based on the configured **Move Restrict**. If you would like to set custom rules (such as requiring the first move to be a rotation), you must configure this table appropriately.

- **Center Restrict**: Center rotation settings. By default only **Empty‑Empty** is checked, which means the center must return to the initial state—the state of the center after the user‑defined **Scramble** and **Rotation**.

- **Max Rotation Count**: The maximum number of times rotations (**x, y, z**) are used, excluding user‑defined **Rotation** and **Pre move**. **The default value is 0**. If rotations (**x, y, z**) are permitted in **Move Restrict**, this must be set to 1 or higher.

- **Max Move Count**: A table of maximum move counts allowed by the **Move Restrict**. Note that the algorithm specified in **Pre Move** is not counted.

<br>

# **Solver Descriptions and Examples**

## **F2L Lite**
A set of solvers for Rubik's Cube cross, XCross, XXCross, XXXCross, and XXXXCross. It is used to search solutions for cross, XCross, F2L, multi-slotting, and more. The **F2L** solver can perform the same search. Here are some examples. 

- Search solutions for yellow cross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=U-_B_R-_L-_F_U-_F2_D_R2_D2_B_L2_F2_U2_D2_F_D2_F-_U2_R2_%252F%252F_setup%250A&index=1&sol=B2_U-_L_F-_D2_L_)]

- Search solutions for white cross, when selecting a rotation alg from the **Rotation**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=D2_F_L2_D_B2_L2_F2_R2_D2_B2_D2_U_F2_U2_B-_L_R_D_R-_F-_U_%252F%252F_setup%250A&index=1&sol=z2_D_R_D2_L2_R-_B-_&rot=z2)]

- Search solutions for white cross, when input a rotation alg in the **Scramble**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=D2_F_L2_D_B2_L2_F2_R2_D2_B2_D2_U_F2_U2_B-_L_R_D_R-_F-_U_%252F%252F_setup%250A%250Az2_%252F%252F_inspection&index=1&sol=D_R_D2_L2_R-_B-_)]

- Search solutions for white XCross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=D-_L-_U2_B_L_B2_R-_D_B2_R_D_R2_D2_F2_R2_L2_F2_D-_F2_D_L2_%252F%252F_setup%250A&index=1&sol=z2_L_R2_B2_U-_L_B-_R_F2_D-_&rot=z2&slot=BL)]

- Search solutions for 1st F2L, setting **Move Restrict** to **URF**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F-_B-_U-_D2_F2_B_U-_R2_F-_R2_B2_D2_L2_U2_L_B2_L-_U2_B2_L2_F2_%252F%252F_setup%250A&index=1&sol=F2_R-_F-_U-_R_F-_&slot=BR&res=URF)]

- Search solutions for white XXCross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F2_U2_F-_R2_F_L2_F2_R2_U2_L2_F-_R2_U_F2_R_B2_F-_U2_F_R-_F2_%252F%252F_setup%250A&index=1&sol=z2_U_B2_U_B-_U_B2_R-_U-_F2_L2_D-_&slot=BL_FL&rot=z2)]

- Search solutions for white XXXCross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=R_F-_L-_U-_F-_U2_D_R_U2_F2_R2_U_L2_D-_F2_R2_D-_L2_D-_F2_B-_%252F%252F_setup%250A&index=1&sol=z2_D-_L_R-_U-_L2_D_F_L_R2_F_R2_D2_&slot=BL_BR_FL&rot=z2)]

- Search solutions for white XXXXCross (F2L skip). [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=D-_B-_R_F2_D-_L-_F-_U-_L_U2_L2_D-_R2_F2_R2_F2_U2_B2_U_R2_%252F%252F_setup%250A&index=1&sol=z2_D_B_U_B-_U-_R-_F_L_F2_D_L2_F2_R-_&slot=BL_BR_FR_FL&rot=z2)]

- Search solutions for last two F2L slots, setting **Move Restrict** to **URF**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=B-_U-_L2_D-_R_F_U_F_B_D2_F_D2_R2_F2_D2_B-_L2_U2_F2_D-_%252F%252F_setup&index=1&sol=U_F-_U-_R-_F_R-_F-_R2_U_F_&slot=BL_BR_FR_FL&res=UDRF)]

## **Pairing**
A set of solvers for a free pair. It is used to search solutions for cross with a free pair, advanced F2L setup, and more. Here are some examples. 

- Search solutions for cross with a free pair. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=L2_D_R_F_L2_F2_D2_L-_F-_B2_U-_L2_D_B2_L2_D_L2_B2_U2_B2_R-_%252F%252F_setup%250A&index=1&sol=B2_U2_L2_R_F_D_&solver=F2L_pair)]

- Search solutions for 3rd F2L and last pair, setting **Move Restrict** to **URF**.  [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=R_D-_R_U_D2_L-_B_L2_U2_D2_R_F2_R-_D2_R_B2_R-_D2_R-_D_%252F%252F_setup%250A&index=1&sol=U2_R-_F-_U-_F2_R-_F-_R2_&solver=F2L_pair&slot2=BL_BR_FL&res=URF)]

## **Pseudo F2L Lite**
A set of solvers for pseudo cross, XCross, XXCross, and XXXCross. It is used to search solutions for pseudo XCross, pseudo slotting, and more. Here are some examples.

- Search solutions for pseudo XCross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=L2_F_L-_F_R2_D-_L_U2_B_L-_D2_B2_R2_L_D2_R_U2_D2_F2_U2_R_%252F%252F_setup%250A&index=1&sol=U-_F2_L_U_R_B2_D2_B_&solver=PF2L&pse=BL)]

- Search solutions for adjecent pseudo XXCross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F_R2_L_U_L2_U2_D_L_U_B_U2_B_R2_B_L2_U2_F-_R2_L2_U2_R2_%252F%252F_setup%250A&index=1&sol=B-_D-_L_R2_B-_L2_R-_D-_R_&solver=PF2L&pse=BL_BR&psc=BL_FL)]

- Search solutions for diagonal pseudo XXCross. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=U-_D2_B2_R_F-_D_R_B2_U_R2_U_L2_D-_R2_U-_L2_U-_L2_B2_R-_%252F%252F_setup%250A&index=1&sol=z2_U2_L-_U-_F2_U2_B-_D2_F_L2_&solver=PF2L&pse=BL_FR&rot=z2&psc=BR_FL)]

## **Pseudo Pairing**
A set of solvers for a pseudo free pair. It is used to search solutions for pseudo cross with a pseudo free pair,  some advanced pseudo slotting, and more. Here are some examples. 

- Search solutions for pseudo cross with a pseudo free pair. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=L_U2_L_B_D2_R_F-_B2_U2_L2_D-_B2_U_B2_R2_U-_R2_F2_R2_B_U2_%252F%252F_setup%250A&index=1&sol=D_B_L-_B2_D_R_&solver=PF2L_pair&apsc=BR)]

- Search solutions for 3rd pseudo slot and last pseudo pair, setting **Move Restrict** to **UDR**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=U-_F_U-_B2_U_F-_B2_D_F2_U-_F2_D_R2_D-_L2_U-_L2_U_%252F%252F_setup%250A&index=1&sol=D_R2_U-_R2_U_R-_U2_R-_&solver=PF2L_pair&pse2=BL_BR_FL&res=UDR)]

## **EOCross**
A set of solvers for EOcross, XEOcross, XXEOcross, XXXEOcross, and XXXXEOCross. Here are some examples.

- Search solutions for EOCross (LR). [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=R-_F_D-_B_D2_L2_B2_D-_L-_D2_B_U2_L2_F-_L2_F-_R2_F_L2_U2_B_%252F%252F_setup%250A&index=1&sol=z2_F-_D-_B-_D2_L-_B_L2_&solver=EOCross&rot=z2)]

- Search solutions for EOCross (FB). [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F-_D-_B2_D_B_D_R_U_F2_R-_F2_R_L2_F2_U2_R2_U2_R_B2_R-_U_%252F%252F_setup%250A&index=1&sol=z2_y-_U_L-_D_F_R_B2_&solver=EOCross&rot=z2_y-)]

- Search solutions for XEOCross (LR). [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F-_D-_F_R2_D-_B_L-_F_U_L2_D_R2_D2_L2_D_B2_R2_U_L2_D2_R-_%252F%252F_setup%250A&index=1&sol=U2_B2_D_R-_U2_F_D_L-_B-_D-_&solver=EOCross&slot3=BL)]

- Search solutions for XXEOCross (FB). [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=U2_D-_F-_U_R2_L_U-_R_U2_R-_B2_L-_F2_L-_D2_R_F2_B2_D_%252F%252F_setup%250A&index=1&sol=y_L2_F2_R2_U2_L-_U-_F_L2_U-_L_F2_&solver=EOCross&slot3=BL_FL&rot=y)]


## **LL Substeps Lite**
A solver for last layer **CP**, **CO**, **EP**, and **EO**. It is used to search solutions for OLL, COLL, ZBLS, and more. The **LL Substeps** solver can perform the same search.

- Search solutions for OLL, setting **Move Restrict** to **URF**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=R_F_R2_F-_R_U_R2_U_F2_L2_B2_D-_B2_L2_F2_U_R2_%252F%252F_setup%250A&index=1&sol=R-_F_U_F2_U_F2_U2_F-_U_R_&solver=LS&ll=CO_EO&res=URF)]

- Search solutions for ZBLS, setting **Move Restrict** to **URF**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F-_U_L-_U2_L_F-_R2_U2_F2_U_F2_L2_U_R2_U-_L2_U_F2_%252F%252F_setup%250A&index=1&sol=F_U2_F2_U-_F2_U-_R-_F-_R_&solver=LS&ll=EO&res=URF)]

- Search solutions for COLL, setting **Move Restrict** to **URF**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=U2_R_U2_R2_F2_D2_L2_D_L2_D_F2_R_%252F%252F_setup%250A&index=1&sol=U2_F_U2_F-_R-_U_F_U_F-_U-_R_&solver=LS&ll=CP_CO_EO&res=URF)]


## **LL Lite**
A solver for last layer. Note that AUF can remain. It is used to search solutions for PLL, 2GLL, ZBLL, 1LLL, and more Here are some examples. The **LL** solver can perform the same search.

- Search solutions for PLL, setting **Move Restrict** to **URF**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=F2_D_B2_D-_L2_U_B2_D_B2_U-_F2_U_L_R-_U2_L-_R-_%252F%252F_setup%250A&index=1&sol=U_R2_U-_R2_U-_R2_U_F_U_F-_R2_F_U-_F-_&solver=LL&res=URF)]

- Search solutions for 2GLL, setting **Move Restrict** to **UR**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=L2_D-_F2_L2_D_F2_R2_B2_U2_F2_U-_L-_B2_R_U2_L-_D2_R_%252F%252F_setup%250A&index=1&sol=U-_R_U2_R-_U-_R_U-_R2_U2_R_U_R-_U_R_&solver=LL&res=UR)]

- Search solutions for PLL-Aa, setting **Move Restrict** to **UDR** and **Pre Move** to **x**.  [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=x_R2-_D2-_R_U_R-_D2-_R_U-_R_x-_%252F%252F_setup&rest=R_R2_R-_U_U2_U-_D_D2_D-&index=1&sol=x_R-_U_R-_D2_R_U-_R-_D2_R2_&solver=LL&res=UDR&premove=x&crest=x_EMPTY)]

- Search solutions for PLL-Na, setting **Move Restrict** to **UDRz**, using the custom **Move Available table**. [[See on the website](https://or18.github.io/RubiksSolverDemo/?scramble=R_U_R-_U2-_R_U_R2-_F-_R_U_R_U-_R-_F_R_U-_R-_U-_R_U-_R-_%252F%252F_setup&rest=U_U2_U-_D_D2_D-_R_R2_R-_z&index=1&sol=z_U_R-_D_R2_U-_R_U_D-_R-_D_R2_U-_R_D-_&solver=LL&res=UDRz&crest=z_EMPTY&mav=U~D2%7CU2~D%7CU2~D2%7CU2~D-%7CU2~z%7CU-~D2%7CU-~z%7CD~z%7CD2~z%7CD-~z%7CR~z%7CR2~z%7CR-~z%7CEMPTY~U2%7CEMPTY~U-%7CEMPTY~D%7CEMPTY~D2%7CEMPTY~D-%7CEMPTY~R%7CEMPTY~R2%7CEMPTY~R-&rc=1)]

## **LL AUF Lite**
A solver for last layer and AUF. The **LL AUF** solver can perform the same search. It is recommended to use **LL Lite** or **LL** solver.

# **Trainers Overview**
Random pattern generator based on [min2phase.js](https://github.com/cs0x7f/min2phase.js) and a full pruning table. 
- The following 6 trainers are currently available.
  - [**Cross trainer**](https://or18.github.io/RubiksSolverDemo/cross_trainer)
  - [**XCross trainer**](https://or18.github.io/RubiksSolverDemo/xcross_trainer)
  - [**Free Pair trainer**](https://or18.github.io/RubiksSolverDemo/pairing_trainer)
  - [**Pseudo XCross trainer**](https://or18.github.io/RubiksSolverDemo/pseudo_xcross_trainer)
  - [**Pseudo Free Pair trainer**](https://or18.github.io/RubiksSolverDemo/pseudo_pairing_trainer)
  - [**EOCross trainer**](https://or18.github.io/RubiksSolverDemo/eocross_trainer)
- Pressing the **[Next]** button generates the next scramble.
- During the initial scramble generation, a pruning table and a pattern table are created, allowing for fast scramble generation in subsequent uses by reusing these tables. 
- Scrambles can be generated by specifying parameters such as rotation, slot, and length of solution. A sticker mask for rendering is generated based on the input parameters. 
- Each trainer comes with a corresponding solver, and you can instantly check the solution by pressing the **[Solve]** button.

Summarize the number of patterns that can be solved with a specific number of moves (in HTM) for each trainer.

- **Cross**

| HTM | Number of Patterns | Percentage |
| --- | ---| ---|
| 0 | 1 | 0.00|
| 1 | 15 | 0.01|
| 2 | 158 | 0.08|
| 3 | 1394 | 0.73|
| 4 | 9809 | 5.16|
| **5** | 46381 | **24.40**|
| **6** | 97254 | **51.16**|
| **7** | 34966 | **18.40**|
| 8 | 102 | 0.05|
<br>

- **XCross**

| HTM | Number of Patterns | Percentage |
| --- | ---| ---|
| 0 | 1 | 0.00|
| 1 | 15 | 0.00|
| 2 | 172 | 0.00|
| 3 | 1950 | 0.00|
| 4 | 21535 | 0.03|
| 5 | 220368 | 0.30|
| 6 | 1989591 | 2.73|
| **7** | 13431990 | **18.40**|
| **8** | 40963892 | **56.12**|
| **9** | 16325184 | **22.37**|
| 10 | 36022 | 0.05|
<br>

- **Free Pair**

| HTM | Number of Patterns | Percentage |
| --- | ---| ---|
| 0 | 17 | 0.00|
| 1 | 255 | 0.00|
| 2 | 3102 | 0.00|
| 3 | 35217 | 0.05|
| 4 | 367070 | 0.50|
| 5 | 3184390 | 4.36|
| **6** | 18621816 | **25.51**|
| **7** | 41028188 | **56.21**|
| **8** | 9746797 | **13.35**|
| 9 | 3868 | 0.01|
<br>

- **Pseudo XCross**

| HTM | Number of Patterns | Percentage |
| --- | ---| ---|
| 0 | 4 | 0.00|
| 1 | 48 | 0.00|
| 2 | 568 | 0.00|
| 3 | 6556 | 0.01|
| 4 | 70495 | 0.10|
| 5 | 693185 | 0.95|
| 6 | 5618257 | 7.70|
| **7** | 27845257 | **38.15**|
| **8** | 36570024 | **50.10**|
| 9 | 2186315 | 3.00|
| 10 | 11 | 0.00|
<br>

- **Pseudo Free Pair**

| HTM | Number of Patterns | Percentage |
| --- | ---| ---|
| 0 | 68 | 0.00|
| 1 | 816 | 0.00|
| 2 | 9256 | 0.01|
| 3 | 103681 | 0.14|
| 4 | 1012687 | 1.39|
| **5** | 7689281 | **10.53**|
| **6** | 32089788 | **43.96**|
| **7** | 30868369 | **42.29**|
| 8 | 1216774 | 1.67|
<br>

- **EOCross**

| HTM | Number of Patterns | Percentage |
| --- | ---| ---|
| 0 | 1 | 0.00|
| 1 | 15 | 0.00|
| 2 | 178 | 0.00|
| 3 | 1982 | 0.01|
| 4 | 21041 | 0.09|
| 5 | 204732 | 0.84|
| 6 | 1645039 | 6.76|
| **7** | 8477633 | **34.84**|
| **8** | 12917628 | **53.09**|
| 9 | 1061851 | 4.36|
| 10 | 140 | 0.00|
