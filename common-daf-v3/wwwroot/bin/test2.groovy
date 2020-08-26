p = TestObject.testParam

header = """
<html><body><h1>Test Page #2 from Groovy</h1><br/>
"""

body = """
hello from a better test.groovy<p>
"""

for (i in 1..10) {
    println "number = ${i}"
}

footer = """
</body>
</html>
"""

response = header + body + footer

