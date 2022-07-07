#include <xinu.h>
#include <future.h>

/*------------------------------------------------------------------------
 * FUTURE MEMORY ALLOCATION
 *------------------------------------------------------------------------*/
future_t* future_alloc(future_mode_t mode, uint size, uint nelem) {
    intmask mask;
    mask = disable();
    
    future_t *f;
    f = (future_t *)getmem((size * nelem) + sizeof(future_t));

    if(mode == FUTURE_SHARED) {
        f->get_queue = newqueue();
    }
    
    if(mode == FUTURE_QUEUE) {
        f->get_queue = newqueue();
        f->set_queue = newqueue();
        f->count = 0;
        f->max_elems = nelem;
        f->head = 0;
        f->tail = 0;
    }

    f->size = size;
	f->state = FUTURE_EMPTY;
    f->mode = mode;
    f->data = sizeof(future_t) + (char *)f;

    restore(mask);
    return f;
}

/*------------------------------------------------------------------------
 * FUTURE MEMORY FREE
 *------------------------------------------------------------------------*/
syscall future_free(future_t* f) {
    if(f->mode == FUTURE_EXCLUSIVE) {    
        kill(f->pid);
        return freemem((char*)(f), f->size + sizeof(future_t));
    }

    if(f->mode == FUTURE_SHARED) {
        delqueue(f->get_queue);
        return freemem((char*)(f), f->size + sizeof(future_t));
    }

    if(f->mode == FUTURE_QUEUE) {
        delqueue(f->get_queue);
        delqueue(f->set_queue);
        return freemem((char*)(f), f->size + sizeof(future_t));
    }
    return SYSERR;
}

/*------------------------------------------------------------------------
 * FUTURE MEMORY GET
 *------------------------------------------------------------------------*/
syscall future_get(future_t* f, char* out) {

    intmask	mask;
	mask=disable();

    // ==============================================================
    // FUTURE_EXCLUSIVE
	if(f->state == FUTURE_EMPTY && f->mode == FUTURE_EXCLUSIVE){
		f->state = FUTURE_WAITING;
        f->pid = getpid();
        suspend(f->pid);
        memcpy(out, f->data, sizeof(f->data));
        
        restore(mask);
	    return OK;
	}

    if (f->state == FUTURE_READY && f->mode == FUTURE_EXCLUSIVE){
        memcpy(out, f->data, sizeof(f->data));
        f->state = FUTURE_EMPTY;

        restore(mask);
	    return OK;
    }

    if(f->state == FUTURE_WAITING && f->mode == FUTURE_EXCLUSIVE){
		restore(mask);
		return SYSERR;
	}

    // ==============================================================
    // FUTURE_SHARED
    if((f->state == FUTURE_EMPTY || f->state == FUTURE_WAITING) && f->mode == FUTURE_SHARED){
		f->state = FUTURE_WAITING;

        enqueue(getpid(), f->get_queue);
        suspend(getpid());
        memcpy(out, f->data, sizeof(f->data));

        restore(mask);
	    return OK;
	}

    if (f->state == FUTURE_READY && f->mode == FUTURE_SHARED){
        f->state = FUTURE_READY;
        memcpy(out, f->data, sizeof(f->data));

        restore(mask);
	    return OK;
    }

    // ==============================================================
    // FUTURE_QUEUE
    if(f->mode == FUTURE_QUEUE){
        if(f->count == 0) {
            enqueue(getpid(), f->get_queue);
            suspend(getpid());
        }
        char* headelemptr = f->data + (f->head * f->size);
        
        memcpy(out, headelemptr, f->size);
        f->head = (f->head + 1) % f->max_elems;
        f->count = f->count - 1;

        // RESUME ONE THREAD FROM SET QUEUE
        pid32 pid = dequeue(f->set_queue);
        resume(pid);

        restore(mask);
        return OK;
	}

    // ==============================================================
    restore(mask);
    return OK;
}

/*------------------------------------------------------------------------
 * FUTURE MEMORY SET
 *------------------------------------------------------------------------*/
syscall future_set(future_t* f, char* in) {
    
    intmask	mask;
	mask=disable();

    // ==============================================================
    // FUTURE_EXCLUSIVE
	if (f->state == FUTURE_EMPTY && f->mode == FUTURE_EXCLUSIVE){
		memcpy(f->data, in, sizeof(in));
        f->state = FUTURE_READY;
        restore(mask);
	    return OK;
	}

    if(f->state == FUTURE_WAITING && f->mode == FUTURE_EXCLUSIVE){
        memcpy(f->data, in, sizeof(in));
        f->state = FUTURE_EMPTY;
        resume(f->pid);
        restore(mask);
	    return OK;
	}

	if(f->state == FUTURE_READY && f->mode == FUTURE_EXCLUSIVE){
		restore(mask);
		return SYSERR;
	}

    // ==============================================================
    // FUTURE_SHARED
    if ((f->state == FUTURE_EMPTY || f->state == FUTURE_WAITING) && f->mode == FUTURE_SHARED){
		memcpy(f->data, in, sizeof(in));
        f->state = FUTURE_READY;

        pid32 pid = dequeue(f->get_queue);
        while(pid != EMPTY) {
            resume(pid);
            pid = dequeue(f->get_queue);   
        }

        restore(mask);
	    return OK;
	}
    
    if(f->state == FUTURE_READY && f->mode == FUTURE_SHARED){
		restore(mask);
		return SYSERR;
	}

    // ==============================================================
    // FUTURE_QUEUE
    if(f->mode == FUTURE_QUEUE){
        if(f->count == (f->max_elems)) {
            enqueue(getpid(), f->set_queue);
            suspend(getpid());
        }
        
        char* tailelemptr = f->data + (f->tail * f->size);
        
        memcpy(tailelemptr, in, f->size);
        f->tail = (f->tail + 1) % f->max_elems;
        f->count = f->count + 1;

        // RESUME ONE THREAD FROM GET QUEUE
        pid32 pid = dequeue(f->get_queue);
        resume(pid);

        restore(mask);
        return OK;
	}    

    // ==============================================================
	restore(mask);
	return OK;
}