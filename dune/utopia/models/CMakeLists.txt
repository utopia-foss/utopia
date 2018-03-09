# Set the models here
set(MODELS
    dummy
    # ... add other models here
    )

# NOTE Do not change below this line ------------------------------------------
# -----------------------------------------------------------------------------

# Add the models as targets
foreach(model ${MODELS})
    add_model(${model} ${model}.cc)
    message(STATUS "Added Utopia model executable:  ${model}")
endforeach(model)