# Loops
You can tell the Noja interpreter to execute a block of code iteratively based on the result of an expression using while and do-while statements. 

The condition may be an expression of any kind, but must always evaluate to a boolean value.

## While loops
A while statement has the following form:
```py
while condition: {
    doSomething();
    doSomethingElse();
}
``` 
as you can see, it doesn't need an ending `;`.

when the interpreter encounters a while statement, it evaluates the condition. If the condition is true, it executes it's body. When the execution of the inner block of code is completed, the execution jumps back to the condition, evaluating it again. If it's again true, the inner code is executed again, else the execution jumps after the while statement. This mechanism can go on potentially for ever!

Like for if-else statements, when the inner block only has one statement, you can drop the `{}`
```py
while condition:
    doSomething();
doSomethingElse(); # This isn't in the while loop's body!
```

## Do-while loops
Do-while loops behave very similarly to while loops, but have minor differences. Do-while loops evaluate the condition *after* each iteration.

They're used like this
```py
do {
    doSomething();
    doSomethingElse();
} while condition;
```
or, if the body only has one statement, like this
```py
do
    doSomething();
while condition;
```

Unlike while loops, do-while loops execute at least once and must end with a `;`.

## Scoping
Loops don't create new variable scopes. When defining a variable in a loop statement (either in the condition or the body) they're defined relative to the loop's parent scope.

## Break jumps
Inside any type of loop, it's always possible to break out of it using the `break` statement. 

```js
while condition: {
    
    if shouldStopLoopin():
        break;

    doSomething()
}
```
the `break` statement will immediately move the execution to after the while or do-while statement.

When inside multiple nested loops, the `break` statement will refer to the inner loop.