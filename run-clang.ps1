

$cppfiles = Get-ChildItem . -Filter *.cpp -Recurse | Where-Object { $_.FullName -notmatch "deps" }
foreach($f in $cppfiles) { clang-format -i -style=llvm $f.FullName }