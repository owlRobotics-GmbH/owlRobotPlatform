# async I/O hello world

# https://docs.python.org/3/library/asyncio.html
# https://docs.python.org/3/library/asyncio-task.html


import asyncio

async def main():
    print('Hello ...')
    await asyncio.sleep(1)
    print('... World!')

asyncio.run(main())


