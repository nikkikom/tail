from behave import given, when, then

import subprocess
from subprocess import call, Popen, PIPE

@given(u'a file with {lines:d} lines')
def step_impl(context,lines):
  context.lines = lines

@when(u'each line length is {length:d}')
def step_impl(context,length):
  file = open("input.txt", "w")
  proc = subprocess.Popen(
    ["../../bin/gen_text", "random:"+str(context.lines)+":"+str(length)],
    stdout=file,stderr=PIPE)
  outs,errs = proc.communicate()
  status = proc.wait()
  if errs:
    raise Exception("Error "+errs)
  if status:
    raise Exception("Error "+str(status))
  pass

@then(u'it returns last {returns:d} lines')
def step_impl(context,returns):
  proc = subprocess.Popen(
    ["bash", "-c", "./check_tail.sh -n 5 -r "+str(returns)+" input.txt"],
    stdout=PIPE,stderr=PIPE)
  outs,errs = proc.communicate()
  status = proc.wait()
  if errs:
    raise Exception("Error "+errs)
  if status:
    raise Exception("Error "+str(status))
  assert status == 0
  # assert context.last_lines == outs
  #assert context.failed is False
  # raise NotImplementedError(u'STEP: Then send to it the file')

