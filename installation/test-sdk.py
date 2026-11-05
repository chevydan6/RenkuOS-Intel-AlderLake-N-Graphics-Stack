#!/usr/bin/env python3
"""Exercise the official MCP client against the provided stdio command."""
import asyncio,json,sys
from mcp import Client,StdioServerParameters
if len(sys.argv)<2:raise SystemExit('provide server command and arguments')
async def main():
 async with Client(StdioServerParameters(command=sys.argv[1],args=sys.argv[2:])) as client:
  tools=await client.list_tools()
  assert any(tool.name=='system_info' for tool in tools.tools)
  result=await client.call_tool('system_info',{})
  info=json.loads(result.content[0].text)
  assert info['sysname']=='Haiku' and info['machine']=='x86_64'
  print('Official MCP SDK over SSH passed:',info['sysname'],info['release'],info['machine'])
asyncio.run(main())
