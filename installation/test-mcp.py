#!/usr/bin/env python3
"""Exercise MCP framing, initialization, discovery, tools, and errors."""
import json,select,subprocess,sys
cmd=sys.argv[1:]
if not cmd:raise SystemExit('provide server command')
p=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,bufsize=0)
def request(data):
 p.stdin.write((json.dumps(data,separators=(',',':'))+'\n').encode());p.stdin.flush()
def response():
 if not select.select([p.stdout],[],[],20)[0]:raise RuntimeError('server response timed out')
 line=p.stdout.readline()
 if not line:raise RuntimeError('server closed stdout')
 return json.loads(line)
try:
 p.stdin.write(b'{invalid\n');p.stdin.flush();assert response()['error']['code']==-32700
 request({'jsonrpc':'2.0','id':1,'method':'tools/list'});assert response()['error']['code']==-32002
 request({'jsonrpc':'2.0','id':2,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'test','version':'1'}}})
 x=response();assert x['id']==2 and x['result']['protocolVersion']=='2025-06-18'
 request({'jsonrpc':'2.0','method':'notifications/initialized'})
 request({'jsonrpc':'2.0','id':'list','method':'tools/list'});x=response();assert x['id']=='list' and x['result']['tools'][0]['name']=='system_info'
 request({'jsonrpc':'2.0','id':4,'method':'tools/call','params':{'name':'system_info','arguments':{}}});x=response();assert x['id']==4 and json.loads(x['result']['content'][0]['text'])['machine']
 request({'jsonrpc':'2.0','id':5,'method':'tools/call','params':{'name':'system_info','arguments':{'unexpected':1}}});assert response()['error']['code']==-32602
 request({'jsonrpc':'2.0','id':6,'method':'unknown'});assert response()['error']['code']==-32601
 request({'jsonrpc':'2.0','id':7,'method':'ping'});assert response()['result']=={}
 p.stdin.close();assert p.wait(timeout=10)==0
 print('MCP initialization, discovery, system_info, errors, ping and stdio shutdown passed')
finally:
 if p.poll() is None:p.kill();p.wait()
