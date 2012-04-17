" vim BTEQ wrapper

function! BTEQ_getLogon()
    while !exists("g:bteq_instance") || empty(g:bteq_instance)
        let g:bteq_instance = input("Enter Teradata Instance: ")
    endwhile

    while !exists("g:bteq_user") || empty(g:bteq_user)
        let g:bteq_user = substitute(system("whoami"), "\n", "", "g")
        let l:change_user = input('Logon with "' . g:bteq_user . '" ? [Y/n] ', "Y")
        if l:change_user == "N" || l:change_user == "n"
            let g:bteq_user = input("Enter BTEQ user: ")
        endif
    endwhile

    while !exists("g:bteq_passwd") || empty(g:bteq_passwd)
        let g:bteq_passwd = inputsecret("Enter BTEQ password: ")
    endwhile

    echo g:bteq_instance
    echo g:bteq_user
    echo g:bteq_passwd
endfunction
        
function! BTEQ_execSQL()
endfunction 
