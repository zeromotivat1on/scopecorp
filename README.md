# Description
The Project I am working on.\
The Game is **NOT** done, a **LOT** may and will change both in design and tech terms.
## Design features
- PS1 style game with huge inspiration from [Petscop](https://youtube.com/playlist?list=PLIEbITAtGBebWGyBZQlkiMXwt30WqG9Bd&si=ecCb_7B7ttppPLUT) series.
- Puzzle oriented spooky adventure.
- Represents corrupted cartridge, disk.\
  Looks fine at first, but after some playtime its clealy seen that something wrong with it.
- 3D Person view and movement.
- Camera follows the player and can be static, with different POV, location, rotations etc. at some levels, areas.
- Taking advantage of game glitches to progress in game.
## Technical features
- For now, the source is **NOT** cross platform/compiler.\
  Working environment is *Win32 | MSVC | C++17 | OpenGL*
- Almost no modern features are used, more C-style.
- I try to remain source as simple as possible to understand, extend, modify and debug.\
  Yet preserving reasonable performance.
- Some language fundamentals are altered or added like:
    - ```defer``` keyword, so you can call something like:
    ```cpp
    const auto file = os_file_open(path);
    defer { os_file_close(file); }
    ```
    - ```Source_Location``` implementation using builtins.
- Almost all systems available are implemented from scratch. No libs, headers or std dependencies (except glad, OpenAL, stb_).
- Data oriented design is prevailed with focus on actual problem solving in favor of unnecessary abstractions.
- General OS api abstraction (win32 for now).
- Allocation is done using either main linear or frame storage, which is always linear ```alloc``` or ```free```.\
  Actual memory allocation is done one at the start of game init, virtual address space is reserved and commited.
- Simple text-based custom file types for shaders (.glsl), materials (.mat), meshes (.mesh).
- Asset registry with serialization to custom asset pack and assets hot reload.
# Preview
- In-game editor with opened runtime profiler and basic dev info.
![image](https://github.com/user-attachments/assets/f5a19d37-5dba-451a-b108-b7186dc4dab1)
- Test scene with post-process
![image](https://github.com/user-attachments/assets/dbbb7a09-790c-441f-8bc7-8fccfa6a97c1)
