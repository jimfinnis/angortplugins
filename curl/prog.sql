create table axes (
    session char(128) not null,
    value char(128) not null,
    axis int not null
    );
    
create table done (
    session char(128) not null,
    value1 char(128) not null,
    value2 char(128) not null,
    status int,
    primary key(session,value1,value2)
    );
    
    
