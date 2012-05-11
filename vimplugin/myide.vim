" {{{
function! XX_CompileCode()
    let ft = &filetype
    execute "compiler " . ft
    silent make
    copen
endfunction
" }}}

" {{{
function! XX_ExecuteCode()
    let ft = &filetype
    if ft == "perl"
        setlocal makeprg=perl\ %
    endif
    silent make
    copen
endfunction
" }}}

" {{{
function! XX_ExecuteSQL()
    while exists("w:db_driver") == 0
        let w:db_driver = input("DB Driver: ")
    endwhile

    if w:db_driver ==? "postgresql" || w:db_driver ==? "pg"
        call XX_GetDBLogon()
        while exists("w:db_name") == 0
            let w:db_name = input("DB Name: ")
        endwhile
        let tempf = tempname()
        silent execute "write ! psql -U " . w:db_user . " -d " . w:db_name . " > " . tempf
        silent execute "split " . tempf
    elseif w:db_driver ==? "teradata" || w:db_driver ==? "td"
        while exists("w:db_host") == 0
            let w:db_host = input("DB Host: ")
        endwhile
        call XX_GetDBLogon()
        let tempf = tempname()
        let templ = tempname()
        silent execute "write ! echo .logon " . w:db_host . "/" . w:db_user . "," . w:db_pass . " > " . tempf
        silent execute "write ! echo .set width 9999 >> " . tempf
        silent execute "write ! echo .set errorout stdout >> " . tempf
        silent execute "write >> " . tempf
        silent execute "write ! bteq < " . tempf . " > " . templ
        silent execute "split " . templ
    endif
endfunction
" }}}

" {{{
function! XX_GetDBLogon()
    while exists("w:db_user") == 0
        let w:db_user = input("DB Username: ")
    endwhile

    if exists("w:db_pass") == 0
        let w:db_pass = escape(inputsecret("DB Password: "), "%#!$")
    endif
endfunction
" }}}

" {{{
function! XX_UnsetDBLogon()
    if exists("w:db_driver")
        unlet w:db_driver
    endif
    if exists("w:db_user")
        unlet w:db_user
    endif
    if exists("w:db_pass")
        unlet w:db_pass
    endif
    if exists("w:db_name")
        unlet w:db_name
    endif
    if exists("w:db_host")
        unlet w:db_host
    endif
endfunction
" }}}

" {{{
map <F6> : call XX_CompileCode()<CR>
map <F5> : call XX_ExecuteCode()<CR>
map <F12> <C-W><C-O>: call XX_ExecuteSQL()<CR>
command RESET call XX_UnsetDBLogon()
" }}}

